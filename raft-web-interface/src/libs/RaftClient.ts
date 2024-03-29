import { BehaviorSubject, distinctUntilChanged, filter, map, Observable, Subject, tap, timestamp } from "rxjs";
import {
  IConnectionStatusServerMessage,
  IConnectionType,
  IDataUpdate,
  IDataUpdateOutgoingMessage,
  IDetailsUpdateServerMessage,
  IServerMessageType,
  IServerOutgoingMessageType,
  IServerState,
  IServerStatusValue,
  ISetServerStatusOutgoingMessage,
  ServerMessage,
} from "../customTypes/server";

export type IServerStatus = Record<number, IServerStatusValue>;
export type IServerStates = Record<number, IServerState>;

class RaftClient {
  // @ts-ignore
  wsClient: WebSocket;

  private latestMessagesSubject: Subject<ServerMessage>;

  latestConnectionStatus: Observable<IConnectionType>;

  private latestShouldResetSubject: Subject<void>;

  latestShouldReset: Observable<void>;

  private latestPausedSubject: BehaviorSubject<boolean>;

  latestPaused: Observable<boolean>;

  latestDetailsUpdateMessages: Observable<IDetailsUpdateServerMessage & { timestamp: number; time: number }>;

  private latestServerStatusSubject: BehaviorSubject<IServerStatus>;

  latestServerStatus: Observable<IServerStatus>;

  private latestServerStateSubject: BehaviorSubject<IServerStates>;

  latestServerState: Observable<IServerStates>;

  private isConnectionStatusMessage(it: ServerMessage): it is IConnectionStatusServerMessage {
    return it.message_type === IServerMessageType.CONNECTION_STATUS
  }

  // @ts-ignore
  private startTime: Date;

  static initialServerStatus: IServerStatus = {
    0: IServerStatusValue.HALTED,
    1: IServerStatusValue.HALTED,
    2: IServerStatusValue.HALTED,
    3: IServerStatusValue.HALTED,
    4: IServerStatusValue.HALTED,
  };

  constructor() {
    this.latestServerStatusSubject = new BehaviorSubject<IServerStatus>(
      RaftClient.initialServerStatus
    );

    this.latestServerStatus = this.latestServerStatusSubject.asObservable();

    this.latestServerStateSubject = new BehaviorSubject<IServerStates>({
      0: IServerState.CANDIDATE,
      1: IServerState.CANDIDATE,
      2: IServerState.CANDIDATE,
      3: IServerState.CANDIDATE,
      4: IServerState.CANDIDATE,
    });

    this.latestServerState = this.latestServerStateSubject.asObservable();

    this.latestShouldResetSubject = new Subject<void>();

    this.latestShouldReset = this.latestShouldResetSubject.asObservable();

    this.latestPausedSubject = new BehaviorSubject<boolean>(false);

    this.latestPaused = this.latestPausedSubject.asObservable();

    this.latestMessagesSubject = new Subject<ServerMessage>();
    this.latestDetailsUpdateMessages = this.latestMessagesSubject.pipe(
      filter((it): it is IDetailsUpdateServerMessage => {
        return it.message_type === IServerMessageType.DETAILS_UPDATE
      }),
      tap((it) => {
        if (it.data.action === 'Status Change: Halted') {
          this.updateLatestServerStatus(it.data.id, IServerStatusValue.HALTED);
        } else {
          this.updateLatestServerStatus(it.data.id, IServerStatusValue.RESTARTED);
        }
      }),
      timestamp(),
      map(({ timestamp, value }) => ({
        ...value,
        timestamp,
        time: ((timestamp.valueOf() - this.startTime.valueOf()) / 100)
      })),
      tap((it) => this.updateLatestServerState(it.data.id, it.data.state)),
      filter(() => !this.latestPausedSubject.getValue())
    );

    this.latestConnectionStatus = this.latestMessagesSubject.pipe(
      map((it) => {
        if (this.isConnectionStatusMessage(it)) {
          return it.data;
        }
        return IConnectionType.STARTED;
      })
    );

    this.latestMessagesSubject.subscribe((it) => {
      if (this.isConnectionStatusMessage(it) && it.data === IConnectionType.STARTED) {
        this.startTime = new Date();
        this.latestShouldResetSubject.next();
        this.latestServerStatusSubject.next(RaftClient.initialServerStatus);
      }
    })

    this.connect();
  }

  private connect() {
    this.wsClient = new WebSocket(
      process.env.REACT_APP_WEBSOCKET_URL ?? 'ws://localhost:8001/'
    );

    this.startTime = new Date();
    this.wsClient.onopen = () => {
      this.startTime = new Date();
      this.latestMessagesSubject.next({
        message_type: IServerMessageType.CONNECTION_STATUS,
        data: IConnectionType.STARTED,
      })
      console.log('ws opened');
    };
    this.wsClient.onclose = () => {
      console.log('ws closed');
      // emulate connection ended
      this.latestMessagesSubject.next({
        message_type: IServerMessageType.CONNECTION_STATUS,
        data: IConnectionType.ENDED,
      })
    };
    
    this.wsClient.onmessage = (msg) => {
      const parsed = JSON.parse(msg.data) as ServerMessage;
      this.latestMessagesSubject.next(parsed);
    }    
  }

  private updateLatestServerStatus(server_id: number, stopped: IServerStatusValue) {
    const currentValue = this.latestServerStatusSubject.getValue();
    if (currentValue[server_id] === stopped) {
      return;
    }
    this.latestServerStatusSubject.next({
      ...currentValue,
      [server_id]: stopped,
    });
  }

  private updateLatestServerState(server_id: number, state: IServerState) {
    const currentValue = this.latestServerStateSubject.getValue();
    if (currentValue[server_id] === state) {
      return;
    }
    this.latestServerStateSubject.next({
      ...currentValue,
      [server_id]: state,
    });
  }

  getLatestServerStatusById(server_id: number): Observable<IServerStatusValue> {
    return this.latestServerStatus.pipe(
      map((state) => state[server_id]),
      distinctUntilChanged()
    )
  }

  togglePause() {
    this.latestPausedSubject.next(!this.latestPausedSubject.getValue())
  }
  
  private setServerStatus(server_id: number, stopped: IServerStatusValue) {
    const message: ISetServerStatusOutgoingMessage = {
      message_type: IServerOutgoingMessageType.SET_SERVER_STATUS,
      data: {
        stopped,
        server_id,
      }
    }
    this.wsClient.send(JSON.stringify(message))
  }

  restartRaft() {
    const message = {
      message_type: IServerOutgoingMessageType.RESTART,
    }
    this.startTime = new Date();
    if (this.wsClient.readyState !== this.wsClient.CLOSED) {
      this.latestMessagesSubject.next({
        message_type: IServerMessageType.CONNECTION_STATUS,
        data: IConnectionType.ENDED,
      })
      this.wsClient.send(JSON.stringify(message));
    } else {
      this.connect();
    }
    this.latestShouldResetSubject.next();
    // Reconnect if Raft is restarted
    this.latestPausedSubject.next(false);
    // Reset server status
    this.latestServerStatusSubject.next(RaftClient.initialServerStatus);
  }

  startServer(server_id: number) {
    this.setServerStatus(server_id, IServerStatusValue.RESTARTED);
  }

  stopServer(server_id: number) {
    this.setServerStatus(server_id, IServerStatusValue.HALTED);
  }

  updateData(update: IDataUpdate) {
    const message: IDataUpdateOutgoingMessage = {
      message_type: IServerOutgoingMessageType.DATA_UPDATE,
      data: update
    }
    this.wsClient.send(JSON.stringify(message))
  }

  close() {
    this.wsClient.close();
  }
}

export const raftClient = new RaftClient()

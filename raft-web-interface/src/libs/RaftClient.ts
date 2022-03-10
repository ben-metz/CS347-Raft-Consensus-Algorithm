import { BehaviorSubject, filter, map, Observable, Subject, switchMap, timestamp } from "rxjs";
import { IConnectionStatusServerMessage, IConnectionType, IDataUpdate, IDataUpdateOutgoingMessage, IDetailsUpdateServerMessage, IServerMessageType, IServerOutgoingMessageType, IServerStatusValue, ISetServerStatusOutgoingMessage, ServerMessage } from "../customTypes/server";

export type IServerStates = Record<number, IServerStatusValue>;

class RaftClient {
  wsClient: WebSocket;

  private latestMessagesSubject: Subject<ServerMessage>;

  latestConnectionStatus: Observable<IConnectionType>;

  private latestShouldResetSubject: Subject<void>;

  latestShouldReset: Observable<void>;

  private latestPausedSubject: BehaviorSubject<boolean>;

  latestPaused: Observable<boolean>;

  latestDetailsUpdateMessages: Observable<IDetailsUpdateServerMessage>;

  private latestServerStateSubject: BehaviorSubject<IServerStates>;

  latestServerState: Observable<IServerStates>;

  private isConnectionStatusMessage(it: ServerMessage): it is IConnectionStatusServerMessage {
    return it.message_type === IServerMessageType.CONNECTION_STATUS
  }
  private startTime: Date;

  constructor() {
    this.startTime = new Date();
    // TODO: Add mechanism to detect initial state of server
    this.latestServerStateSubject = new BehaviorSubject<IServerStates>({
      0: IServerStatusValue.RESTARTED,
      1: IServerStatusValue.RESTARTED,
      2: IServerStatusValue.RESTARTED,
      3: IServerStatusValue.RESTARTED,
      4: IServerStatusValue.RESTARTED,
    });

    this.latestServerState = this.latestServerStateSubject.asObservable();

    this.latestShouldResetSubject = new Subject<void>();

    this.latestShouldReset = this.latestShouldResetSubject.asObservable();

    this.latestPausedSubject = new BehaviorSubject<boolean>(false);

    this.latestPaused = this.latestPausedSubject.asObservable();

    this.wsClient = new WebSocket(
      process.env.REACT_APP_WEBSOCKET_URL ?? 'ws://localhost:8001/'
    );
    this.wsClient.onopen = () => {
      console.log('ws opened');
    };
    this.wsClient.onclose = () => console.log('ws closed');
    this.latestMessagesSubject = new Subject<ServerMessage>();

    this.wsClient.onmessage = (msg) => {
      const parsed = JSON.parse(msg.data) as ServerMessage;
      this.latestMessagesSubject.next(parsed);
    }

    this.latestDetailsUpdateMessages = this.latestMessagesSubject.pipe(
      filter((it): it is IDetailsUpdateServerMessage => {
        return it.message_type === IServerMessageType.DETAILS_UPDATE
      }),
      switchMap((it) => {
        return this.latestPaused.pipe(
          filter((paused) => !paused),
          map(() => it),
          timestamp(),
          map(({ timestamp, value }) => ({
            ...value,
            timestamp,
            time: ((timestamp.valueOf() - this.startTime.valueOf()) / 100)
          })),    
        )
      }),
    );

    this.latestConnectionStatus = this.latestMessagesSubject.pipe(
      map((it) => {
        if (this.isConnectionStatusMessage(it)) {
          return it.data;
        }
        return IConnectionType.STARTED;
      })
    );
  }

  private updateLatestServerState(server_id: number, stopped: IServerStatusValue) {
    const currentValue = this.latestServerStateSubject.getValue();
    if (currentValue[server_id] === stopped) {
      return;
    }
    this.latestServerStateSubject.next({
      ...currentValue,
      [server_id]: stopped,
    });
  }

  getLatestServerStateById(server_id: number): Observable<IServerStatusValue> {
    return this.latestServerState.pipe(
      map((state) => state[server_id])
    )
  }

  togglePause() {
    this.latestPausedSubject.next(!this.latestPausedSubject.getValue())
  }
  
  private setServerState(server_id: number, stopped: IServerStatusValue) {
    this.updateLatestServerState(server_id, stopped);
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
    if (this.wsClient.readyState !== this.wsClient.CLOSED) {
      this.wsClient.send(JSON.stringify(message))      
    } else {
      this.wsClient = new WebSocket(
        process.env.REACT_APP_WEBSOCKET_URL ?? 'ws://localhost:8001/'
      );
    }
    this.startTime = new Date();
    this.latestShouldResetSubject.next();
  }

  startServer(server_id: number) {
    this.setServerState(server_id, IServerStatusValue.RESTARTED);
  }

  stopServer(server_id: number) {
    this.setServerState(server_id, IServerStatusValue.HALTED);
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

  getLatestMessagesByServer(id: number) {
    return this.latestDetailsUpdateMessages.pipe(
      filter((it) => it.data.id === id)
    )
  }
}

export const raftClient = new RaftClient()

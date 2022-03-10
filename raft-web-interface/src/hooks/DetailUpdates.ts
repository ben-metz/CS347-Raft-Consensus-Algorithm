import { filter, Observable, Subject } from "rxjs";
import { IDetailsUpdateServerMessage } from "../customTypes/server";

class DetailUpdates {
  wsClient: WebSocket;

  private latestMessagesSubject: Subject<IDetailsUpdateServerMessage>;

  latestMessages: Observable<IDetailsUpdateServerMessage>;

  constructor() {
    this.wsClient = new WebSocket('ws://localhost:8001/');
    this.wsClient.onopen = () => {
      console.log('ws opened');
    };
    this.wsClient.onclose = () => console.log('ws closed');
    this.latestMessagesSubject = new Subject<IDetailsUpdateServerMessage>();

    this.wsClient.onmessage = (msg) => {
      const parsed = JSON.parse(msg.data) as IDetailsUpdateServerMessage;
      this.latestMessagesSubject.next(parsed);
    }

    this.latestMessages = this.latestMessagesSubject.asObservable();
  }

  close() {
    this.wsClient.close();
  }

  getLatestMessagesByServer(id: number) {
    return this.latestMessages.pipe(
      filter((it) => it.data.id === id)
    )
  }
}

export default DetailUpdates;

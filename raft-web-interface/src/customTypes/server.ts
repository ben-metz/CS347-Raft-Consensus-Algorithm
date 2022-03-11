export enum IServerState {
  LEADER = 0,
  CANDIDATE = 1,
  FOLLOWER = 2
}

export enum IServerMessageType {
  REQUEST_VOTE = 'request_vote',
  APPEND_ENTRIES = 'append_entries',
  CONNECTION_STATUS = 'connection_status',
  DETAILS_UPDATE = 'details_update',
}

export enum IServerOutgoingMessageType {
  SET_SERVER_STATUS = 'set_server_status',
  DATA_UPDATE = 'data_update',
  RESTART = 'restart',
  SET_TIMEOUT = 'set_timeout',
}

export enum IConnectionType {
  ENDED = 'ended',
  STARTED = 'started'
}

export interface IDetailsUpdate {
  action: string;
  database: string;
  id: number;
  lastCommited: number;
  state: IServerState;
  term: number;
  vote: number;
}

export interface IEntry {
  term: number;
  index: number;
  value: number;
}

export interface IAppendEntriesFail {
  term: number;
  leaderId: number;
  prevLogIndex: number;
  prevLogTerm: number;
  leaderCommit: number;
  entries: IEntry[];
}

export interface IServerMessage<T> {
  message_type: IServerMessageType;
  data: T;
}

export interface IDetailsUpdateServerMessage extends IServerMessage<IDetailsUpdate> {
  message_type: IServerMessageType.DETAILS_UPDATE;
}

export interface IConnectionStatusServerMessage extends IServerMessage<IConnectionType> {
  message_type: IServerMessageType.CONNECTION_STATUS;
}

export interface IAppendEntriesFailServerMessage extends IServerMessage<IAppendEntriesFail> {
  sender_id: number;
  response: 'false';
  message_type: IServerMessageType.APPEND_ENTRIES;
}

export interface IAppendEntriesSuccess {
  prevLogIndex: number;
  term: number;
  success: boolean;
}

export interface IAppendEntriesSuccessServerMessage extends IServerMessage<IAppendEntriesSuccess> {
  sender_id: number;
  response: 'true';
  message_type: IServerMessageType.APPEND_ENTRIES;
}

export interface IRequestVote {
  term: number;
  candidate_id: number;
  last_log_index: number;
  last_log_term: number;
}

export interface IRequestVoteServerMessage extends IServerMessage<IRequestVote> {
  message_type: IServerMessageType.REQUEST_VOTE;
}

export enum IServerStatusValue {
  RESTARTED = 0,
  HALTED = 1
}

export interface ISetServerStatus {
  server_id: number;
  stopped: IServerStatusValue;
}

export interface ISetServerStatusOutgoingMessage {
  message_type: IServerOutgoingMessageType.SET_SERVER_STATUS;
  data: ISetServerStatus;
}

export interface IDataUpdate {
  server_id: number;
  index: number;
  value: number;
}

export interface IDataUpdateOutgoingMessage {
  message_type: IServerOutgoingMessageType.DATA_UPDATE;
  data: IDataUpdate;
}

export interface ISetTimeout {
  timeout: number;
}

export interface ISetTimeoutOutgoingMessage {
  message_type: IServerOutgoingMessageType.SET_TIMEOUT;
  data: ISetTimeout;
}

export type ServerMessage = IAppendEntriesFailServerMessage
  | IAppendEntriesSuccessServerMessage
  | IDetailsUpdateServerMessage
  | IRequestVoteServerMessage
  | IConnectionStatusServerMessage;

export interface IServerGridData {
  state: IServerState,
  term: number;
  vote: number;
  action: string;
  array: string;
  commit: number;
  time: number;
}

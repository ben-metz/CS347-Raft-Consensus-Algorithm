import { FC } from "react";
import { Button } from "react-bootstrap";
import { IServerStatusValue } from "../customTypes/server";
import { useObservableState } from "observable-hooks";
import { raftClient } from "libs/RaftClient";
import { useCallback } from "react";

const getMessage = (status?: IServerStatusValue) => {
  switch (status) {
    case IServerStatusValue.RESTARTED: return "Stop";
    default: return "Start";
  }
}

const getButtonClass = (status?: IServerStatusValue) => {
  switch (status) {
    case IServerStatusValue.RESTARTED: return "bg-red-500 hover:bg-red-700 text-white font-bold py-2 px-4 rounded disabled:bg-slate-300";
    default: return "bg-green-500 hover:bg-green-700 text-white font-bold py-2 px-4 rounded disabled:bg-slate-300";
  }
}

export interface IServerConnectionButtonProps {
  serverId: number;
}

const ServerConnectionButton: FC<IServerConnectionButtonProps> = ({
  serverId,
}) => {
  const [serverState] = useObservableState(() => raftClient.getLatestServerStateById(serverId));
  const onClick = useCallback(() => {
    if (serverState === IServerStatusValue.HALTED) {
      raftClient.startServer(serverId);
    } else {
      raftClient.stopServer(serverId);
    }
  }, [serverState, serverId]);

  return (
    <Button onClick={onClick} className={getButtonClass(serverState)}>
      {getMessage(serverState)}
    </Button>
  );
}

export default ServerConnectionButton;
import { FC } from "react";
import { Button } from "react-bootstrap";
import { IConnectionType } from "../customTypes/server";

const getMessage = (status: IConnectionType) => {
  switch (status) {
    case IConnectionType.STARTED: return "Stop";
    case IConnectionType.ENDED: return "Start";
    default: throw new Error('Invalid server status!')
  }
}

const getButtonClass = (status: IConnectionType) => {
  switch (status) {
    case IConnectionType.STARTED: return "bg-red-500 hover:bg-red-700 text-white font-bold py-2 px-4 rounded disabled:bg-slate-300";
    case IConnectionType.ENDED: return "bg-green-500 hover:bg-green-700 text-white font-bold py-2 px-4 rounded disabled:bg-slate-300";
    default: throw new Error('Invalid server status!')
  }
}

export interface IServerConnectionButtonProps {
  status: IConnectionType;
  serverId: number;
}

const ServerConnectionButton: FC<IServerConnectionButtonProps> = ({
  status,
  serverId,
}) => {
  return (
    <Button className={getButtonClass(status)} disabled>
      {getMessage(status)}
    </Button>
  );
}

export default ServerConnectionButton;

import { IConnectionType } from "customTypes/server";
import { raftClient } from "libs/RaftClient";
import { useObservableState } from "observable-hooks";
import { FC } from "react";
import ConnectionButton from "./ConnectionButton";

const getStatusMessage = (status?: IConnectionType) => {
  if (status === IConnectionType.STARTED) {
    return "Connected";
  }
  return "Disconnected";
}

const getStatusClass = (status?: IConnectionType) => {
  if (status === IConnectionType.STARTED) {
    return "text-green-500";
  }
  return "text-red-500";
}

const ConnectionStatus: FC = () => {
  const [status] = useObservableState(() => raftClient.latestConnectionStatus);

  return (
    <div className="mt-2">
      <div>
        <p className="text-center font-bold text-xl">Connection Status: <span className={getStatusClass(status)}>{getStatusMessage(status)}</span></p>
      </div>
      <div className="text-center mt-2">
        <button className="mr-2 bg-red-500 hover:bg-red-700 text-white font-bold py-2 px-4 rounded disabled:bg-slate-300" onClick={() => raftClient.restartRaft()}>
          Restart
        </button>
        <ConnectionButton />
      </div>
    </div>
  )
};

export default ConnectionStatus;

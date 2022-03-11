import { IConnectionType } from "customTypes/server";
import useLeaderServerId from "hooks/useLeaderServerId";
import usePaused from "hooks/usePaused";
import { raftClient } from "libs/RaftClient";
import { useObservableState } from "observable-hooks";
import { FC, useMemo } from "react";
import { distinctUntilChanged } from "rxjs";
import ConnectionButton from "./ConnectionButton";

const getStatusMessage = (connected: boolean) => {
  if (connected) {
    return "Connected";
  }
  return "Disconnected";
}

const getStatusClass = (connected: boolean) => {
  if (connected) {
    return "text-green-500";
  }
  return "text-red-500";
}

const CurrentLeaderText: FC = () => {
  const leaderServerId = useLeaderServerId();

  return <span>{leaderServerId !== null ? `Server ${leaderServerId}` : 'None'}</span>
}

const CurrentLeader: FC = () => {
  return (
    <p className="text-center font-bold text-xl">Current Leader 👑: <CurrentLeaderText /></p>
  );
}

const ConnectionStatusText: FC = () => {
  const [status] = useObservableState(() => raftClient.latestConnectionStatus.pipe(distinctUntilChanged()));
  const paused = usePaused();

  const connected = useMemo(() => status === IConnectionType.STARTED && !paused, [status, paused]);

  return (
    <span className={getStatusClass(connected)}>{getStatusMessage(connected)}</span>
  )
}

const ConnectionStatus: FC = () => {
  return (
    <div className="mt-1 sticky top-0 bg-white dark:bg-black py-2 z-50 flex-row flex items-center justify-center">
      <div className="mr-6">
        <p className="text-center font-bold text-xl">Connection Status: <ConnectionStatusText /></p>
        <CurrentLeader />
      </div>
      <div className="text-center">
        <button className="mr-2 bg-red-500 hover:bg-red-700 text-white font-bold py-2 px-4 rounded disabled:bg-slate-300" onClick={() => raftClient.restartRaft()}>
          Restart
        </button>
        <ConnectionButton />
      </div>
    </div>
  )
};

export default ConnectionStatus;

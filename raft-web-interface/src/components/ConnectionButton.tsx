import { IConnectionType } from "customTypes/server";
import usePaused from "hooks/usePaused";
import { raftClient } from "libs/RaftClient";
import { useObservableState } from "observable-hooks";
import { FC } from "react";

const getMessage = (paused: boolean) => (paused ? "Resume" : "Pause");
const getColor = (connected: boolean) => (connected ? "bg-green-500 hover:bg-green-700" : "bg-red-500 hover:bg-red-700")

const ConnectionButton: FC = () => {
  const paused = usePaused();
  const [status] = useObservableState(() => raftClient.latestConnectionStatus);

  if (status === IConnectionType.ENDED) {
    return null;
  }

  return (
    <button onClick={() => raftClient.togglePause()} className={`${getColor(paused)} text-white font-bold py-2 px-4 rounded disabled:bg-slate-300`}>
      {getMessage(paused)}
    </button>
  )
}

export default ConnectionButton;

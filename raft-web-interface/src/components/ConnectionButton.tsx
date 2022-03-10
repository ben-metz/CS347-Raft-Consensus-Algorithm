import { raftClient } from "libs/RaftClient";
import { useObservableState } from "observable-hooks";
import { FC } from "react";

const getMessage = (paused: boolean) => (paused ? "Connect" : "Disconnect");
const getColor = (connected: boolean) => (connected ? "bg-green-500 hover:bg-green-700" : "bg-red-500 hover:bg-red-700")

const ConnectionButton: FC = () => {
  let paused = useObservableState(raftClient.latestPaused);
  paused = paused ?? false;

  return (
    <button onClick={() => raftClient.togglePause()} className={`${getColor(paused)} text-white font-bold py-2 px-4 rounded disabled:bg-slate-300`}>
      {getMessage(paused)}
    </button>
  )
}

export default ConnectionButton;

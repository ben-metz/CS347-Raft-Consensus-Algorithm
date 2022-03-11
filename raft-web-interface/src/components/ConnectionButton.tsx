import usePaused from "hooks/usePaused";
import { raftClient } from "libs/RaftClient";
import { FC } from "react";

const getMessage = (paused: boolean) => (paused ? "Connect" : "Disconnect");
const getColor = (connected: boolean) => (connected ? "bg-green-500 hover:bg-green-700" : "bg-red-500 hover:bg-red-700")

const ConnectionButton: FC = () => {
  const paused = usePaused();

  return (
    <button onClick={() => raftClient.togglePause()} className={`${getColor(paused)} text-white font-bold py-2 px-4 rounded disabled:bg-slate-300`}>
      {getMessage(paused)}
    </button>
  )
}

export default ConnectionButton;

import { raftClient } from "libs/RaftClient";
import { useObservableState } from "observable-hooks";
import { useMemo } from "react";

const usePaused: () => boolean = () =>{
  const paused = useObservableState(raftClient.latestPaused);
  
  return useMemo(() => paused ?? false, [paused]);
}

export default usePaused;

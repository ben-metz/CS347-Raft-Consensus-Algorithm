import { IConnectionType } from "customTypes/server";
import usePaused from "hooks/usePaused";
import { raftClient } from "libs/RaftClient";
import { useObservableState } from "observable-hooks";

const useDisableCommands = (): boolean => {
  const [status] = useObservableState(() => raftClient.latestConnectionStatus);
  const paused = usePaused();

  return paused || status === IConnectionType.ENDED;
}

export default useDisableCommands;

import { IServerState } from "customTypes/server"
import { raftClient } from "libs/RaftClient"
import { useObservableState } from "observable-hooks"
import { distinctUntilChanged, filter, map } from "rxjs"

const useLeaderServerId = (): number | null => {
  const [leaderServerId] = useObservableState(() => raftClient.latestDetailsUpdateMessages.pipe(
    filter((it) => it.data.state === IServerState.LEADER),
    map((it) => it.data.id),
    distinctUntilChanged(),
  ));

  return leaderServerId ?? null;
}

export default useLeaderServerId;
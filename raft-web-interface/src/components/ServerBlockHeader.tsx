import { IServerState } from "customTypes/server";
import { raftClient } from "libs/RaftClient";
import { useObservableState } from "observable-hooks";
import { FC } from "react";
import { distinct, filter, map, tap } from "rxjs";
import DuplicatedMessagesButton from "./DuplicatedMessagesButton";
import GridClearButton from "./GridClearButton";
import ServerConnectionButton from "./ServerStatusButton";

interface IServerBlockHeaderProps {
  showDuplicated?: boolean;
  serverId: number;
  toggleShowDuplicated: () => void;
  onGridClear: () => void;
}

const ServerCurrentTime: FC<{ serverId: number }> = ({ serverId }) => {
  const [currentTime] = useObservableState(() => raftClient.latestDetailsUpdateMessages.pipe(
    filter((it) => it.data.id === serverId),
    map((it) => it.time),
    distinct()
  ))

  return (
    <p className="text-xl text-center mb-2">(Current Time: {currentTime?.toFixed(2)} ms)</p>
  )
}

const ServerIsLeader: FC<{ serverId: number }> = ({ serverId }) => {
  const [isLeader] = useObservableState(() => raftClient.latestDetailsUpdateMessages.pipe(
    filter((it) => it.data.id === serverId),
    map((it) => it.data.state),
    tap((it) => {
      console.log(serverId, it)
    }),
    map((it) => it === IServerState.LEADER)
  ));

  if (!isLeader) {
    return null;
  }

  return (
    <span>(Leader 👑)</span>
  )
}

const ServerBlockHeader: FC<IServerBlockHeaderProps> = ({
  showDuplicated,
  serverId,
  toggleShowDuplicated,
  onGridClear
}) => {
  return (
    <>
      <h2 className='text-center font-bold text-2xl'>Server {serverId + 1} <ServerIsLeader serverId={serverId} /></h2>
      <ServerCurrentTime serverId={serverId} />
      <div className="mb-2 flex justify-between">
        <DuplicatedMessagesButton serverId={serverId} showDuplicated={showDuplicated} onClick={toggleShowDuplicated} />
        <GridClearButton onClick={onGridClear} />
        <ServerConnectionButton serverId={serverId} />
      </div>
    </>
  );
}

export default ServerBlockHeader;

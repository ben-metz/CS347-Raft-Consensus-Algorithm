import { FC } from "react";
import DuplicatedMessagesButton from "./DuplicatedMessagesButton";
import ServerConnectionButton from "./ServerStatusButton";

interface IServerBlockHeaderProps {
  showDuplicated?: boolean;
  serverId: number;
  toggleShowDuplicated: () => void;
}

const ServerBlockHeader: FC<IServerBlockHeaderProps> = ({
  showDuplicated,
  serverId,
  toggleShowDuplicated
}) => {
  return (
    <>
      <h2 className='text-center font-bold text-2xl'>Server {serverId + 1}</h2>
      <div className="mb-2 flex justify-between">
        <DuplicatedMessagesButton serverId={serverId} showDuplicated={showDuplicated} onClick={toggleShowDuplicated} />
        <ServerConnectionButton serverId={serverId} />
      </div>
    </>
  );
}

export default ServerBlockHeader;

import { FC, useCallback, useState } from "react";
import { IConnectionType } from "../customTypes/server";
import ServerBlockHeader from "./ServerBlockHeader";
import ServerGrid from "./ServerGrid";

const ServerBlock: FC<{ serverId: number }> = ({ serverId }) => {
  const [showDuplicated, setShowDuplicated] = useState<boolean>(false);

  const toggleShowDuplicated = useCallback(() => {
    setShowDuplicated((show) => !show)
  }, []);

  return (
    <div className="py-2">
      <ServerBlockHeader
        serverId={serverId}
        status={IConnectionType.STARTED}
        showDuplicated={showDuplicated}
        toggleShowDuplicated={toggleShowDuplicated}
      />
      <ServerGrid showDuplicated={showDuplicated} serverId={serverId} />
    </div>
  )
}

export default ServerBlock;
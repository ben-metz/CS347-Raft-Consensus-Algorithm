import { GridApi } from "ag-grid-community";
import { FC, useCallback, useState } from "react";
import ServerBlockHeader from "./ServerBlockHeader";
import ServerGrid from "./ServerGrid";

const ServerBlock: FC<{ serverId: number }> = ({ serverId }) => {
  const [showDuplicated, setShowDuplicated] = useState<boolean>(false);
  const [agGridApi, setAgGridApi] = useState<GridApi>();

  const toggleShowDuplicated = useCallback(() => {
    setShowDuplicated((show) => !show)
  }, []);

  const onGridClear = useCallback(() => {
    if (!agGridApi) {
      return;
    }
    agGridApi.flushAsyncTransactions();
    agGridApi.setRowData([])
  }, [agGridApi]);

  return (
    <div className="py-2">
      <ServerBlockHeader
        serverId={serverId}
        showDuplicated={showDuplicated}
        toggleShowDuplicated={toggleShowDuplicated}
        onGridClear={onGridClear}
      />
      <ServerGrid
        serverId={serverId}
        showDuplicated={showDuplicated}
        agGridApi={agGridApi}
        setAgGridApi={setAgGridApi}
      />
    </div>
  )
}

export default ServerBlock;
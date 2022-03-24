import { FC, memo, useEffect, useRef } from 'react';

import 'ag-grid-community/dist/styles/ag-grid.css';
import 'ag-grid-community/dist/styles/ag-theme-alpine.css';
import { IServerState } from 'customTypes/server';
import { ColDef, GridApi, GridReadyEvent, Grid } from 'ag-grid-community';
import { raftClient } from 'libs/RaftClient';
import { distinctUntilChanged, filter, tap } from 'rxjs';
import { useCallback } from 'react';

const parseState = (val: IServerState) => {
  switch (val) {
    case IServerState.CANDIDATE: return "Candidate";
    case IServerState.FOLLOWER: return "Follower";
    case IServerState.LEADER: return "Leader";
    default: throw new Error("Invalid server state!");
  }
};

export interface IServerGridProps {
  serverId: number;
  showDuplicated: boolean;
  agGridApi?: GridApi;
  setAgGridApi: (api: GridApi) => void;
}

const columnDefs: ColDef[] = [
  {
    field: "time",
    valueGetter: (it) => it.data.time,
    valueFormatter: (value) => value.value.toFixed(2),
    width: 96,
    resizable: true,
  },
  {
    field: "state",
    valueGetter: (it) => parseState(it.data.data.state),
    width: 110,
    resizable: true,
    filter: 'agTextColumnFilter',
  },
  {
    field: "term",
    valueGetter: (it) => it.data.data.term,
    width: 90,
    resizable: true,
    filter: 'agNumberColumnFilter',
  },
  {
    field: "vote",
    valueGetter: (it) => it.data.data.vote,
    width: 90,
    resizable: true,
    filter: 'agNumberColumnFilter',
  },
  {
    field: "action",
    valueGetter: (it) => it.data.data.action,
    width: 250,
    resizable: true,
    filter: 'agTextColumnFilter',
  },
  {
    field: "array",
    valueGetter: (it) => it.data.data.database,
    width: 120,
    resizable: true,
  },
  {
    field: "commit",
    valueGetter: (it) => it.data.data.lastCommited,
    width: 110,
    resizable: true,
    filter: 'agNumberColumnFilter',
  },
];

const ServerGrid: FC<IServerGridProps> = ({
  serverId,
  showDuplicated,
  agGridApi,
  setAgGridApi,
}) => {
  const gridContainerRef = useRef<HTMLDivElement>();
  const resetAll = useCallback(() => {
    if (!agGridApi) {
      return;
    }
    agGridApi.flushAsyncTransactions();
    agGridApi.setRowData([]);
  }, [agGridApi]);

  const onGridReady = useCallback(({ api }: GridReadyEvent) => {
    setAgGridApi(api);
  }, [setAgGridApi]);

  useEffect(() => {
    if (!gridContainerRef.current || Boolean(agGridApi)) {
      return;
    }
    // Use non-framework version of AG Grid since it is more performant
    new Grid(gridContainerRef.current, {
      headerHeight: 36,
      rowHeight: 36,
      onGridReady,
      columnDefs,
    });
  }, [onGridReady, agGridApi]);

  useEffect(() => {
    if (!agGridApi) return;
    const subscription = raftClient.latestDetailsUpdateMessages.pipe(
      filter((it) => it.data.id === serverId),
      !showDuplicated ?
          distinctUntilChanged((prev, next) => {
            if (showDuplicated) {
              return false;
            }
            return prev.message_type === next.message_type
              && prev.data.action === next.data.action
              && prev.data.term === next.data.term
              && prev.data.lastCommited === next.data.lastCommited
              && prev.data.state === next.data.state
              && prev.data.vote === next.data.vote
              && prev.data.database === next.data.database
          }) : tap(),
    ).subscribe((update) => {
      agGridApi.applyTransactionAsync({
        add: [update],
        addIndex: 0,
      });
    });
    const resetSubscription = raftClient.latestShouldReset.subscribe(() => {
      resetAll();
    })
    return () => {
      subscription.unsubscribe();
      resetSubscription.unsubscribe();
    }
  }, [agGridApi, serverId, showDuplicated, resetAll]);

  return (
      <div className="ag-theme-alpine" style={{height: 360, width: '100%'}}>
        <div id="grid" ref={gridContainerRef as any} style={{ height: 360 }} />
      </div>
  );
};

export default memo(ServerGrid);

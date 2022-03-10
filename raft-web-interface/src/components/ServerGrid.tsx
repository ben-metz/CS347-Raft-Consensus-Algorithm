import { AgGridReact } from 'ag-grid-react';
import { FC, useEffect, useState } from 'react';

import 'ag-grid-community/dist/styles/ag-grid.css';
import 'ag-grid-community/dist/styles/ag-theme-alpine.css';
import { IServerState } from 'customTypes/server';
import { ColDef, GridApi } from 'ag-grid-community';
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

const ServerGrid: FC<{ serverId: number; showDuplicated: boolean }> = ({
  serverId,
  showDuplicated
}) => {
  const [agGridApi, setAgGridApi] = useState<GridApi>();
  const [columnDefs] = useState<ColDef[]>([
      {
        field: "time",
        valueGetter: (it) => it.data.time,
        valueFormatter: (value) => value.value.toFixed(2),
        sort: 'desc',
        width: 96,
      },
      {
        field: "state",
        valueGetter: (it) => parseState(it.data.data.state),
        width: 100,
      },
      {
        field: "term",
        valueGetter: (it) => it.data.data.term,
        width: 68,
      },
      {
        field: "vote",
        valueGetter: (it) => it.data.data.vote,
        width: 68
      },
      {
        field: "action",
        valueGetter: (it) => it.data.data.action
        
      },
      {
        field: "array",
        valueGetter: (it) => it.data.data.database,
        width: 96
      },
      {
        field: "commit",
        valueGetter: (it) => it.data.data.lastCommited,
        width: 100,
      },
  ]);

  const resetAll = useCallback(() => {
    if (!agGridApi) {
      return;
    }
    agGridApi.flushAsyncTransactions();
    agGridApi.setRowData([]);
  }, [agGridApi])

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
        add: [update]
      })
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
        <AgGridReact
          columnDefs={columnDefs}
          rowData={[]}
          onGridReady={({ api }) => {
            setAgGridApi(api)
          }}
          gridOptions={{
            headerHeight: 36,
            rowHeight: 36,
          }}
        >
        </AgGridReact>
      </div>
  );
};

export default ServerGrid;

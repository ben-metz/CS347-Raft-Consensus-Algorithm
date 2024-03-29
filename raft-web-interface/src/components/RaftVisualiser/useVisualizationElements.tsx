import { IServerState, IServerStatusValue } from 'customTypes/server';
import useLeaderServerId from 'hooks/useLeaderServerId';
import usePaused from 'hooks/usePaused';
import { raftClient } from 'libs/RaftClient';
import { useObservableState } from 'observable-hooks';
import React from 'react';
import { useEffect } from 'react';
import { FC } from 'react';
import { CSSProperties } from 'react';
import { useState } from 'react';
import { Elements, Position } from 'react-flow-renderer';

const defaultStyle: CSSProperties = {
  width: 80,
  height: 80,
  borderRadius: 80,
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  borderColor: 'black',
  borderWidth: 2,
};

const haltedServerStyle: CSSProperties = {
  ...defaultStyle,
  borderColor: 'red',
  background: '#222222',
  color: '#666666',
}

const candidateStyle: CSSProperties = {
  ...defaultStyle,
  background: '#ff9b94',
}

const followerStyle: CSSProperties = {
  ...defaultStyle,
  background: '#9ef7dd',
}

const leaderStyle: CSSProperties = {
  ...defaultStyle,
  background: '#ffd5ba',
}

const getStateLabel = (state: IServerState): string => {
  if (state === IServerState.CANDIDATE) {
    return "Candidate";
  }
  if (state === IServerState.LEADER) {
    return "Leader\n👑";
  }
  return "Follower";
}

const ServerLabel: FC<{ serverId: number; state: IServerState }> = ({
  serverId,
  state,
}) => (
  <div>
    <p className="font-bold">Server {serverId}</p>
    <p>({getStateLabel(state)})</p>
  </div>
)

const initialElements: Elements = [
  {
    id: '0',
    data: {
      label: 'Server 1',
    },
    type: 'output',
    position: { x: 250, y: 0 },
    style: defaultStyle,
  },
  {
    id: '1',
    type: 'output',
    data: {
      label: 'Server 2',
    },
    position: { x: 100, y: 150 },
    style: defaultStyle,
    targetPosition: Position.Right,
    sourcePosition: Position.Right
  },
  {
    id: '2',
    type: 'output',
    data: {
      label: 'Server 3',
    },
    position: { x: 400, y: 150 },
    style: defaultStyle,
    targetPosition: Position.Left,
    sourcePosition: Position.Left
  },
  {
    id: '3',
    type: 'output',
    data: {
      label: 'Server 4',
    },
    position: { x: 100, y: 300 },
    style: defaultStyle,
    targetPosition: Position.Right,
    sourcePosition: Position.Right
  },
  {
    id: '4',
    type: 'output',
    data: {
      label: 'Server 5',
    },
    position: { x: 400, y: 300 },
    style: defaultStyle,
    targetPosition: Position.Left,
    sourcePosition: Position.Left
  },
];

const NUM_SERVERS = 5;

const getStyle = (state: IServerState, status: IServerStatusValue): CSSProperties => {
  if (status === IServerStatusValue.HALTED) {
    return haltedServerStyle;
  }
  if (state === IServerState.CANDIDATE) {
    return candidateStyle;
  }
  if (state === IServerState.LEADER) {
    return leaderStyle;
  }
  return followerStyle;
}

const useVisualizationElements = () => {
  const [elements, setElements] = useState<Elements>(initialElements);
  const leaderServerId = useLeaderServerId();
  const [serverStates] = useObservableState(() => raftClient.latestServerState);
  const [serverStatus] = useObservableState(() => raftClient.latestServerStatus);
  const paused = usePaused();

  useEffect(() => {
    if (leaderServerId === null || !serverStates || !serverStatus) {
      return;
    }
    const elems: Elements = [];
    for (let i = 0; i < NUM_SERVERS; i++) {
      elems.push({
        ...initialElements[i],
        style: getStyle(serverStates[i], paused ? IServerStatusValue.HALTED : serverStatus[i]),
        type: 'input',
        data: {
          label: <ServerLabel serverId={i} state={serverStates[i]} />
        }
      });
    }
    if (!paused) {
      for (let i = 0; i < NUM_SERVERS; i++) {
        elems.push({
          id: `e${leaderServerId}-${i}`,
          source: `${leaderServerId}`,
          target: `${i}`,
          animated: true,
        })
      }  
    }
    setElements(elems)
  }, [leaderServerId, serverStates, paused, serverStatus]);

  return elements;
}

export default useVisualizationElements;

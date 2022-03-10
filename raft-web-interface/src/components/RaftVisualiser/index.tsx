import React from 'react';
import { FC } from 'react';

import ReactFlow, {
  Controls,
  Background,
  OnLoadFunc,
} from 'react-flow-renderer';
import useVisualizationElements from './useVisualizationElements';

const onLoad: OnLoadFunc = (reactFlowInstance) => {
  console.log('flow loaded:', reactFlowInstance);
  reactFlowInstance.fitView();
};

const RaftVisualiser: FC = () => {
  const elements = useVisualizationElements();

  return (
    <>
      <h2 className="text-center font-bold text-xl mb-2">Server Visualisation</h2>
      <div style={{ height: 500, width: '100%' }}>
        <ReactFlow
          elements={elements}
          onLoad={onLoad}
          zoomOnScroll={false}
          zoomOnDoubleClick={false}
          zoomOnPinch={false}
          nodesDraggable={false}
          snapToGrid={true}
          contentEditable={false}
          snapGrid={[15, 15]}
        >
          <Controls />
          <Background color="#aaa" gap={16} />
        </ReactFlow>
      </div>
    </>
  );
};

export default RaftVisualiser;

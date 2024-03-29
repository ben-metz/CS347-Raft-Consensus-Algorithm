import ConnectionStatus from 'components/ConnectionStatus';
import DataInputForm from 'components/DataInputForm';
import React, { FC } from 'react';
import './App.css';
import 'ag-grid-community/dist/styles/ag-grid.css';
import 'ag-grid-community/dist/styles/ag-theme-alpine.css';
import ServerBlock from './components/ServerBlock';
import RaftVisualiser from 'components/RaftVisualiser';

const App: FC = () => {
  return (
    <div className="container mx-auto py-4 px-6 max-w-7xl text-black dark:text-white" style={{ maxWidth: 1600 }} >
      <h1 className='text-center text-3xl font-bold'>Raft Consensus Algorithm Interface</h1>
      <ConnectionStatus />
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        {Array(5).fill(null).map((_, i) => (
          <ServerBlock serverId={i} key={i} />
        ))}
      </div>
      <DataInputForm />
      <RaftVisualiser />
    </div>
  );
}

export default App;

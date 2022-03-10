import ConnectionStatus from 'components/ConnectionStatus';
import DataInputForm from 'components/DataInputForm';
import React, { FC } from 'react';
import './App.css';
import ServerBlock from './components/ServerBlock';

const App: FC = () => {
  return (
    <div className="container mx-auto py-4 px-6 max-w-6xl">
      <h1 className='text-center text-3xl font-bold'>Raft Consensus Algorithm Interface</h1>
      <ConnectionStatus />
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        {Array(5).fill(null).map((_, i) => (
          <ServerBlock serverId={i} key={i} />
        ))}
      </div>
      <DataInputForm />
    </div>
  );
}

export default App;

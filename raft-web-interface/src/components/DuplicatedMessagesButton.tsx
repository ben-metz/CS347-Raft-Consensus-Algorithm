import { FC, MouseEventHandler } from "react";

interface IDuplicatedMessagesButtonProps {
  showDuplicated?: boolean;
  serverId: number;
  onClick: MouseEventHandler<HTMLButtonElement>;
}

const DuplicatedMessagesButton: FC<IDuplicatedMessagesButtonProps> = ({
  showDuplicated,
  serverId,
  onClick
}) => {
  return (
    <button onClick={onClick} className="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded">
      {showDuplicated ? 'Hide Duplicated Messages' : 'Show Duplicated Messages'}
    </button>
    );
}

export default DuplicatedMessagesButton;

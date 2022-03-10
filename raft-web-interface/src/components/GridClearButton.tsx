import { FC, MouseEventHandler } from "react";

interface IGridClearButtonProps {
  onClick: MouseEventHandler<HTMLButtonElement>;
}

const GridClearButton: FC<IGridClearButtonProps> = ({
  onClick
}) => {
  return (
    <button onClick={onClick} className="bg-cyan-500 hover:bg-cyan-700 text-white font-bold py-2 px-4 rounded">
      Clear Data
    </button>
    );
}

export default GridClearButton;

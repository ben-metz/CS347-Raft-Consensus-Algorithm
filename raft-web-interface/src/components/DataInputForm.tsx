import { IDataUpdate } from "customTypes/server";
import usePaused from "hooks/usePaused";
import { raftClient } from "libs/RaftClient";
import { useCallback } from "react";
import { FC } from "react";
import { useForm } from "react-hook-form";

const DataInputForm: FC = () => {
  const { register, handleSubmit, formState: { errors }, reset } = useForm<IDataUpdate>();
  const paused = usePaused();

  const dataUpdateSubmit = useCallback((values: IDataUpdate) => {
    raftClient.updateData(values);
    reset();
  }, [reset])

  return (
    <div className="max-w-lg mx-auto mt-4">
      <h2 className="text-center font-bold text-xl mb-2">Update Data</h2>
      <form onSubmit={handleSubmit(dataUpdateSubmit)}>
        <div className="mb-8">
          <label
            htmlFor="server_id"
            className={`block font-bold text-sm mb-2 ${
              errors.server_id ? "text-red-600" : ""
            }`}
          >
            Server ID
          </label>
          <input
            type="number"
            id="server_id"
            placeholder="Input a server ID from 0 to 4"
            className={`block w-full bg-transparent outline-none border-b-2 py-2 px-4 ${
              errors.server_id
                ? "text-red-300 border-red-400"
                : ""
            }`}
            {...register('server_id')}
          />
          {errors.server_id && (
            <p className="text-red-500 text-sm mt-2">
              A valid server ID is required.
            </p>
          )}
        </div>
        <div className="mb-8">
          <label
            htmlFor="index"
            className={`block font-bold text-sm mb-2 ${
              errors.index ? "text-red-600" : ""
            }`}
          >
            Index
          </label>
          <input
            type="number"
            id="index"
            placeholder="Input the index of the database that you want to update (0 to 4)"
            className={`block w-full bg-transparent outline-none border-b-2 py-2 px-4 ${
              errors.index
                ? "text-red-300 border-red-400"
                : ""
            }`}
            {...register('index')}
          />
          {errors.index && (
            <p className="text-red-500 text-sm mt-2">
              A valid index is required.
            </p>
          )}
        </div>
        <div className="mb-8">
          <label
            htmlFor="value"
            className={`block font-bold text-sm mb-2 ${
              errors.value ? "text-red-600" : ""
            }`}
          >
            Value
          </label>
          <input
            type="number"
            id="value"
            placeholder="Input an integer value"
            className={`block w-full bg-transparent outline-none border-b-2 py-2 px-4 ${
              errors.value
                ? "text-red-300 border-red-400"
                : ""
            }`}
            {...register('value')}
          />
          {errors.value && (
            <p className="text-red-500 text-sm mt-2">
              A valid value is required.
            </p>
          )}
        </div>
        <button disabled={paused} type="submit" className="inline-block bg-green-700 text-white font-bold rounded shadow py-2 px-5 text-sm disabled:bg-slate-300">
          Update Data
        </button>
      </form>  
    </div>
  )
}

export default DataInputForm;

import { IDataUpdate, ISetTimeout } from "customTypes/server";
import { raftClient } from "libs/RaftClient";
import { useCallback } from "react";
import { FC } from "react";
import { useForm } from "react-hook-form";

const SetTimeoutForm: FC = () => {
  const { register, handleSubmit, formState: { errors }, reset } = useForm<ISetTimeout>();

  const dataUpdateSubmit = useCallback((values: ISetTimeout) => {
    raftClient.setServerTimeout(values.timeout);
    reset();
  }, [reset])

  return (
    <div className="max-w-lg mx-auto mt-4">
      <h2 className="text-center font-bold text-xl mb-2">Update Timeout</h2>
      <form onSubmit={handleSubmit(dataUpdateSubmit)}>
        <div className="mb-8">
          <label
            htmlFor="timeout"
            className={`block font-bold text-sm mb-2 ${
              errors.timeout ? "text-red-600" : ""
            }`}
          >
            Timeout
          </label>
          <input
            type="number"
            id="timeout"
            placeholder="Input a timeout value (in nanoseconds)"
            className={`block w-full bg-transparent outline-none border-b-2 py-2 px-4 ${
              errors.timeout
                ? "text-red-300 border-red-400"
                : ""
            }`}
            {...register('timeout')}
          />
          {errors.timeout && (
            <p className="text-red-500 text-sm mt-2">
              A valid timeout value is required.
            </p>
          )}
        </div>
        <button type="submit" className="inline-block bg-green-700 text-white font-bold rounded shadow py-2 px-5 text-sm">
          Update Timeout
        </button>
      </form>  
    </div>
  )
}

export default SetTimeoutForm;

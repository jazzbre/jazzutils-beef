using System;
using System.Collections;

namespace jazzutils
{
	public class Job
	{
		public void* handle = null;
		public typealias Callback = delegate void(uint32 startIndex, uint32 endIndex, uint32 workerIndex);
		public Callback callback = null ~ delete _;
	}

	public class PinnedJob
	{
		public void* handle = null;
		public typealias Callback = delegate void();
		public Callback callback = null ~ delete _;
	}

	public static class JobSystem
	{
		public typealias JobHandle = void*;
		public typealias JobPinnedHandle = void*;

		private static enkiTaskScheduler* taskScheduler = null;
		private static List<Job> jobs = new .() ~ DeleteContainerAndItems!(_);
		private static List<PinnedJob> pinnedJobs = new .() ~ DeleteContainerAndItems!(_);

		public static bool Initialize(uint32 workerCount)
		{
			taskScheduler = enkiNewTaskScheduler();
			enkiInitTaskSchedulerNumThreads(taskScheduler, Math.Max(workerCount, 2));
			return true;
		}

		public static void Finalize()
		{
			enkiWaitForAll(taskScheduler);
			enkiDeleteTaskScheduler(taskScheduler);
			taskScheduler = null;
		}

		public static int GetWorkerCount()
		{
			return enkiGetNumTaskThreads(taskScheduler);
		}

		static void JobFunction(uint32 startIndex, uint32 endIndex, uint32 workerIndex, void* userData)
		{
			Job job = Internal.UnsafeCastToObject(userData) as Job;
			job.callback(startIndex, endIndex, workerIndex);
		}

		public static Job CreateJob(Job.Callback jobFunction)
		{
			Job job = new .();
			job.handle = enkiCreateTaskSet(taskScheduler, => JobFunction);
			job.callback = jobFunction;
			jobs.Add(job);
			return job;
		}

		public static void AddJob(Job job, uint32 jobCount)
		{
			if (jobCount == 0)
			{
				return;
			}
			enkiAddTaskSetArgs(taskScheduler, job.handle, Internal.UnsafeCastToPtr(job), jobCount);
		}

		public static void WaitJobs(Job job)
		{
			enkiWaitForTaskSet(taskScheduler, job.handle);
		}

		static void PinnedJobFunction(void* userData)
		{
			PinnedJob job = Internal.UnsafeCastToObject(userData) as PinnedJob;
			job.callback();
		}

		public static PinnedJob CreateJobPinned(PinnedJob.Callback jobFunction, uint32 threadIndex)
		{
			PinnedJob job = new .();
			job.handle = enkiCreatePinnedTask(taskScheduler, => PinnedJobFunction, threadIndex);
			job.callback = jobFunction;
			pinnedJobs.Add(job);
			return null;
		}

		public static void AddJobPinned(PinnedJob job)
		{
			enkiAddPinnedTaskArgs(taskScheduler, job.handle, Internal.UnsafeCastToPtr(job));
		}

		public static bool IsJobPinnedDone(PinnedJob job)
		{
			return enkiIsPinnedTaskComplete(taskScheduler, job.handle) == 1;
		}

		public static void WaitJobPinned(PinnedJob job)
		{
			enkiWaitForPinnedTask(taskScheduler, job.handle);
		}

		typealias enkiTaskScheduler = void;
		typealias enkiTaskSet = void;
		typealias enkiPinnedTask = void;
		private typealias enkiTaskExecuteRange = function void(uint32 startIndex, uint32 endIndex, uint32 workerIndex, void* userData);
		private typealias enkiPinnedTaskExecute = function void(void* pArgs_);
		[CLink] private static extern enkiTaskScheduler* 	enkiNewTaskScheduler();
		[CLink] private static extern void 					enkiInitTaskSchedulerNumThreads(  enkiTaskScheduler* pETS_, uint32 numThreads_);
		[CLink] private static extern void 					enkiDeleteTaskScheduler(enkiTaskScheduler* pETS_);
		[CLink] private static extern void 					enkiWaitForAll(enkiTaskScheduler* pETS_);
		[CLink] private static extern uint32            	enkiGetNumTaskThreads(enkiTaskScheduler* pETS_);
		[CLink] private static extern uint32            	enkiGetThreadNum(enkiTaskScheduler* pETS_);
		[CLink] private static extern enkiTaskSet*        	enkiCreateTaskSet(enkiTaskScheduler* pETS_, enkiTaskExecuteRange taskFunc_  );
		[CLink] private static extern void                	enkiDeleteTaskSet(enkiTaskScheduler* pETS_, enkiTaskSet* pTaskSet_);
		[CLink] private static extern void                	enkiAddTaskSetArgs(enkiTaskScheduler* pETS_, enkiTaskSet* pTaskSet_, void* pArgs_, uint32 setSize_);
		[CLink] private static extern void 					enkiWaitForTaskSet(enkiTaskScheduler* pETS_, enkiTaskSet* pTaskSet_);
		[CLink] private static extern enkiPinnedTask* 		enkiCreatePinnedTask(enkiTaskScheduler* pETS_, enkiPinnedTaskExecute taskFunc_, uint32 threadNum_);
		[CLink] private static extern void 					enkiAddPinnedTaskArgs(enkiTaskScheduler* pETS_, enkiPinnedTask* pTask_, void* pArgs_);
		[CLink] private static extern int 					enkiIsPinnedTaskComplete(enkiTaskScheduler* pETS_, enkiPinnedTask* pTask_);
		[CLink] private static extern void 					enkiWaitForPinnedTask(enkiTaskScheduler* pETS_, enkiPinnedTask* pTask_);
	}
}
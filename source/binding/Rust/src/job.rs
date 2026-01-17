//! Asynchronous job management for tracking operation status and results.

use crate::{
    common::{MaaId, MaaStatus},
    sys, MaaResult,
};

/// Thread-safe pointer wrapper for FFI closures.
pub struct SendSyncPtr<T>(pub *mut T);
unsafe impl<T> Send for SendSyncPtr<T> {}
unsafe impl<T> Sync for SendSyncPtr<T> {}

impl<T> SendSyncPtr<T> {
    pub fn new(ptr: *mut T) -> Self {
        Self(ptr)
    }
    pub fn get(&self) -> *mut T {
        self.0
    }
}

impl<T> Clone for SendSyncPtr<T> {
    fn clone(&self) -> Self {
        Self(self.0)
    }
}
impl<T> Copy for SendSyncPtr<T> {}

pub type StatusFn = Box<dyn Fn(MaaId) -> MaaStatus + Send + Sync>;
pub type WaitFn = Box<dyn Fn(MaaId) -> MaaStatus + Send + Sync>;

/// An asynchronous operation handle.
///
/// Use this to track the status of controller, resource, and tasker operations.
///
/// # Example
/// ```ignore
/// let job = controller.post_click(100, 200)?;
/// let status = job.wait(); // Blocks until complete
/// if status.succeeded() {
///     println!("Click successful.");
/// }
/// ```
pub struct Job {
    pub id: MaaId,
    status_fn: StatusFn,
    wait_fn: WaitFn,
}

impl Job {
    /// Create a new Job with custom status/wait functions.
    pub fn new(id: MaaId, status_fn: StatusFn, wait_fn: WaitFn) -> Self {
        Self {
            id,
            status_fn,
            wait_fn,
        }
    }

    pub fn for_tasker(tasker_ptr: *mut sys::MaaTasker, id: MaaId) -> Self {
        let ptr = SendSyncPtr::new(tasker_ptr);
        Self {
            id,
            status_fn: Box::new(move |job_id| {
                MaaStatus(unsafe { sys::MaaTaskerStatus(ptr.get(), job_id) })
            }),
            wait_fn: Box::new(move |job_id| {
                MaaStatus(unsafe { sys::MaaTaskerWait(ptr.get(), job_id) })
            }),
        }
    }

    pub fn for_controller(ctrl_ptr: *mut sys::MaaController, id: MaaId) -> Self {
        let ptr = SendSyncPtr::new(ctrl_ptr);
        Self {
            id,
            status_fn: Box::new(move |job_id| {
                MaaStatus(unsafe { sys::MaaControllerStatus(ptr.get(), job_id) })
            }),
            wait_fn: Box::new(move |job_id| {
                MaaStatus(unsafe { sys::MaaControllerWait(ptr.get(), job_id) })
            }),
        }
    }

    pub fn for_resource(res_ptr: *mut sys::MaaResource, id: MaaId) -> Self {
        let ptr = SendSyncPtr::new(res_ptr);
        Self {
            id,
            status_fn: Box::new(move |job_id| {
                MaaStatus(unsafe { sys::MaaResourceStatus(ptr.get(), job_id) })
            }),
            wait_fn: Box::new(move |job_id| {
                MaaStatus(unsafe { sys::MaaResourceWait(ptr.get(), job_id) })
            }),
        }
    }

    /// Block until the operation completes.
    pub fn wait(&self) -> MaaStatus {
        (self.wait_fn)(self.id)
    }

    /// Get the current status without blocking.
    pub fn status(&self) -> MaaStatus {
        (self.status_fn)(self.id)
    }

    pub fn succeeded(&self) -> bool {
        self.status() == MaaStatus::SUCCEEDED
    }

    pub fn failed(&self) -> bool {
        self.status() == MaaStatus::FAILED
    }

    pub fn running(&self) -> bool {
        self.status() == MaaStatus::RUNNING
    }

    pub fn pending(&self) -> bool {
        self.status() == MaaStatus::PENDING
    }

    pub fn done(&self) -> bool {
        let s = self.status();
        s == MaaStatus::SUCCEEDED || s == MaaStatus::FAILED
    }
}

/// An asynchronous operation handle with typed result retrieval.
///
/// Similar to [`Job`] but includes a `get()` method to retrieve the operation result.
pub struct JobWithResult<T> {
    job: Job,
    get_fn: Box<dyn Fn(MaaId) -> MaaResult<Option<T>> + Send + Sync>,
}

impl<T> JobWithResult<T> {
    /// Create a new JobWithResult with custom status/wait/get functions.
    pub fn new(
        id: MaaId,
        status_fn: StatusFn,
        wait_fn: WaitFn,
        get_fn: impl Fn(MaaId) -> MaaResult<Option<T>> + Send + Sync + 'static,
    ) -> Self {
        Self {
            job: Job::new(id, status_fn, wait_fn),
            get_fn: Box::new(get_fn),
        }
    }

    pub fn id(&self) -> MaaId {
        self.job.id
    }

    pub fn wait(&self) -> MaaStatus {
        self.job.wait()
    }

    pub fn status(&self) -> MaaStatus {
        self.job.status()
    }

    pub fn succeeded(&self) -> bool {
        self.job.succeeded()
    }

    pub fn failed(&self) -> bool {
        self.job.failed()
    }

    pub fn running(&self) -> bool {
        self.job.running()
    }

    pub fn pending(&self) -> bool {
        self.job.pending()
    }

    pub fn done(&self) -> bool {
        self.job.done()
    }

    /// Get the operation result.
    ///
    /// # Arguments
    /// * `wait` - If `true`, blocks until the operation completes before getting the result
    pub fn get(&self, wait: bool) -> MaaResult<Option<T>> {
        if wait {
            self.wait();
        }
        (self.get_fn)(self.job.id)
    }
}

// === Type Aliases for Specialized Jobs ===

/// Controller operation job.
///
/// Returned by controller methods like `post_click()`, `post_swipe()`.
pub type CtrlJob = Job;

/// Resource loading job.
///
/// Returned by resource methods like `post_bundle()`.
pub type ResJob = Job;

/// Task job with result retrieval.
///
/// Returned by `Tasker::post_task()`.
pub type TaskJobWithResult = JobWithResult<crate::common::TaskDetail>;

/// Recognition job with result retrieval.
///
/// Returned by `Tasker::post_recognition()`.
pub type RecoJobWithResult = JobWithResult<crate::common::RecognitionDetail>;

/// Action job with result retrieval.
///
/// Returned by `Tasker::post_action()`.
pub type ActionJobWithResult = JobWithResult<crate::common::ActionDetail>;

pub fn tasker_ptr(ptr: *mut sys::MaaTasker) -> SendSyncPtr<sys::MaaTasker> {
    SendSyncPtr::new(ptr)
}

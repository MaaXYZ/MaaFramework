use crate::define::*;

pub struct Job<S, W>
where
    S: Fn(MaaId) -> MaaStatus,
    W: Fn(MaaId) -> MaaStatus,
{
    id: MaaId,
    status_fn: S,
    wait_fn: W,
}

impl<S, W> Job<S, W>
where
    S: Fn(MaaId) -> MaaStatus,
    W: Fn(MaaId) -> MaaStatus,
{
    pub fn new(id: MaaId, status_fn: S, wait_fn: W) -> Self {
        Self {
            id,
            status_fn,
            wait_fn,
        }
    }

    pub fn id(&self) -> MaaId {
        self.id
    }

    pub fn wait(&self) -> Status {
        Status::from((self.wait_fn)(self.id))
    }

    pub fn status(&self) -> Status {
        Status::from((self.status_fn)(self.id))
    }

    pub fn done(&self) -> bool {
        self.status().done()
    }

    pub fn succeeded(&self) -> bool {
        self.status().succeeded()
    }

    pub fn failed(&self) -> bool {
        self.status().failed()
    }

    pub fn pending(&self) -> bool {
        self.status().pending()
    }

    pub fn running(&self) -> bool {
        self.status().running()
    }
}

pub struct TaskJob {
    id: MaaTaskId,
    handle: MaaTaskerHandle,
}

impl TaskJob {
    pub(crate) fn new(id: MaaTaskId, handle: MaaTaskerHandle) -> Self {
        Self { id, handle }
    }

    pub fn id(&self) -> MaaTaskId {
        self.id
    }

    pub fn wait(&self) -> Status {
        Status::from(crate::ffi::maa_tasker_wait(self.handle, self.id))
    }

    pub fn status(&self) -> Status {
        Status::from(crate::ffi::maa_tasker_status(self.handle, self.id))
    }

    pub fn done(&self) -> bool {
        self.status().done()
    }

    pub fn succeeded(&self) -> bool {
        self.status().succeeded()
    }

    pub fn failed(&self) -> bool {
        self.status().failed()
    }

    pub fn pending(&self) -> bool {
        self.status().pending()
    }

    pub fn running(&self) -> bool {
        self.status().running()
    }
}

pub struct CtrlJob {
    id: MaaCtrlId,
    handle: MaaControllerHandle,
}

impl CtrlJob {
    pub(crate) fn new(id: MaaCtrlId, handle: MaaControllerHandle) -> Self {
        Self { id, handle }
    }

    pub fn id(&self) -> MaaCtrlId {
        self.id
    }

    pub fn wait(&self) -> Status {
        Status::from(crate::ffi::maa_controller_wait(self.handle, self.id))
    }

    pub fn status(&self) -> Status {
        Status::from(crate::ffi::maa_controller_status(self.handle, self.id))
    }

    pub fn done(&self) -> bool {
        self.status().done()
    }

    pub fn succeeded(&self) -> bool {
        self.status().succeeded()
    }

    pub fn failed(&self) -> bool {
        self.status().failed()
    }

    pub fn pending(&self) -> bool {
        self.status().pending()
    }

    pub fn running(&self) -> bool {
        self.status().running()
    }
}

pub struct ResJob {
    id: MaaResId,
    handle: MaaResourceHandle,
}

impl ResJob {
    pub(crate) fn new(id: MaaResId, handle: MaaResourceHandle) -> Self {
        Self { id, handle }
    }

    pub fn id(&self) -> MaaResId {
        self.id
    }

    pub fn wait(&self) -> Status {
        Status::from(crate::ffi::maa_resource_wait(self.handle, self.id))
    }

    pub fn status(&self) -> Status {
        Status::from(crate::ffi::maa_resource_status(self.handle, self.id))
    }

    pub fn done(&self) -> bool {
        self.status().done()
    }

    pub fn succeeded(&self) -> bool {
        self.status().succeeded()
    }

    pub fn failed(&self) -> bool {
        self.status().failed()
    }

    pub fn pending(&self) -> bool {
        self.status().pending()
    }

    pub fn running(&self) -> bool {
        self.status().running()
    }
}


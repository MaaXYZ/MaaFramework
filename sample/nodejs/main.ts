import maa from "../../source/binding/NodeJS/release/maa-node/dist/index-client";
// import maa from '@maaxyz/maa-node'

console.log(maa.Global.version);

const my_reco: maa.CustomRecognizerCallback = async (self) => {
    let reco_detail = await self.context.run_recognition(
        "MyCustomOCR",
        self.image,
        {
            MyCustomOCR: {
                roi: [100, 100, 200, 300],
            },
        }
    );

    self.context.override_pipeline({
        MyCustomOCR: {
            roi: [1, 1, 114, 514],
        },
    });

    const new_context = self.context.clone();
    new_context.override_pipeline({
        MyCustomOCR: {
            roi: [100, 200, 300, 400],
        },
    });

    reco_detail = await new_context.run_recognition("MyCustomOCR", self.image);

    const click_job = self.context.tasker.controller!.post_click(10, 20);
    await click_job.wait();

    self.context.override_next(self.task, ["TaskA", "TaskB"]);

    return [
        {
            x: 0,
            y: 0,
            width: 100,
            height: 100,
        },
        "Hello World!",
    ];
};

async function main() {
    maa.Global.config_init_option("./");

    const res = new maa.Resource();
    res.notify = (msg, detail) => {
        console.log(msg, detail);
    };
    await res.post_bundle("sample/resource").wait();

    const devices = await maa.AdbController.find();
    if (!devices || devices.length === 0) {
        return;
    }
    const [name, adb_path, address, screencap_methods, input_methods, config] =
        devices[0];
    const ctrl = new maa.AdbController(
        adb_path,
        address,
        screencap_methods,
        input_methods,
        config
    );
    ctrl.notify = (msg, detail) => {
        console.log(msg, detail);
    };
    await ctrl.post_connection().wait();

    const tskr = new maa.Tasker();
    tskr.notify = (msg, detail) => {
        console.log(msg, detail);
    };
    // tskr.parsed_notify = (msg) => {
    //     console.log(msg)
    // }

    tskr.bind(ctrl);
    tskr.bind(res);

    console.log(tskr.inited);

    res.register_custom_recognizer("MyRec", my_reco);

    let task_detail = await tskr
        .post_task("StartUpAndClickButton")
        .wait()
        .get();

    tskr.destroy();
    ctrl.destroy();
    res.destroy();
}

main();

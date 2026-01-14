import { join } from 'path'
import './maa-client'

import * as fs from 'fs/promises'

let analyzed = false
let runned = false

const myReco: maa.CustomRecognitionCallback = async self => {
    console.log('on myReco')
    const entry = 'ColorMatch'
    const ppover = {
        ColorMatch: {
            recognition: 'ColorMatch',
            lower: [100, 100, 100],
            upper: [255, 255, 255],
            action: 'Click'
        }
    }

    await self.context.run_task(entry, ppover)
    const action_detail = await self.context.run_action(
        entry,
        [114, 514, 191, 810],
        'RunAction Detail',
        ppover
    )
    console.log('action_detail:', action_detail)
    const reco_detail = await self.context.run_recognition(entry, self.image, ppover)
    console.log('reco_detail:', reco_detail)

    const new_ctx = self.context.clone()
    new_ctx.override_pipeline({
        TaskA: {},
        TaskB: {}
    })
    new_ctx.override_next(self.task, ['TaskA', 'TaskB'])

    const node_id = new_ctx.tasker.latest_node('ColorMatch')
    const node_detail = node_id ? new_ctx.tasker.node_detail(node_id) : null
    console.log('node_detail:', node_detail)

    const new_task_detail = new_ctx.tasker.task_detail(new_ctx.task_id)
    console.log('new_task_detail:', new_task_detail)

    analyzed = true

    return [[11, 4, 5, 14], 'Hello World!']
}

const myAct: maa.CustomActionCallback = async self => {
    console.log('on myAct')
    const ctrl = self.context.tasker.controller
    if (!ctrl) {
        return false
    }
    await ctrl.post_screencap().wait()
    const new_image = ctrl.cached_image
    await ctrl.post_click(191, 98).wait()
    await ctrl.post_swipe(100, 200, 300, 400, 100).wait()
    await ctrl.post_input_text('Hello World!').wait()
    await ctrl.post_click_key(32).wait()
    await ctrl.post_touch_down(1, 100, 100, 0).wait()
    await ctrl.post_touch_move(1, 200, 200, 0).wait()
    await ctrl.post_touch_up(1).wait()
    await ctrl.post_start_app('aaa').wait()
    await ctrl.post_stop_app('bbb').wait()

    const cached_image = ctrl.cached_image
    const connected = ctrl.connected
    const uuid = ctrl.uuid
    ctrl.screenshot_target_long_side = 1080
    ctrl.screenshot_target_short_side = 720

    runned = true

    return true
}

async function api_test() {
    const r1 = new maa.Resource()
    r1.inference_execution_provider = 'DirectML'
    r1.inference_execution_provider = 'CoreML'
    r1.inference_execution_provider = 'Auto'
    r1.destroy()

    const r2 = new maa.Resource()
    r2.inference_execution_provider = 'CPU'
    r2.inference_execution_provider = 'DirectML'
    r2.inference_device = 114514
    r2.inference_execution_provider = 'CPU'
    await r2.post_bundle('/path/to/resource').wait()
    r2.destroy()

    const resource = new maa.Resource()
    resource.add_sink((res, msg) => {
        console.log(msg)
    })
    console.log('rsource', resource)

    const dbg_controller = new maa.DbgController(
        '../../install/test/PipelineSmoking/Screenshot',
        '../../install/test/user',
        maa.DbgControllerType.CarouselImage,
        '{}'
    )
    dbg_controller.add_sink((res, msg) => {
        console.log(msg)
    })
    console.log('controller', dbg_controller)
    await dbg_controller.post_connection().wait()

    const tasker = new maa.Tasker()
    tasker.add_sink(msg => {
        console.log(msg)
    })
    console.log('tasker', tasker)
    tasker.resource = resource
    tasker.controller = dbg_controller

    if (!tasker.inited) {
        console.log('failed to init tasker')
        process.exit(1)
    }

    resource.register_custom_action('MyAct', myAct)
    resource.register_custom_recognition('MyRec', myReco)

    const ppover = {
        Entry: { next: 'Rec' },
        Rec: {
            recognition: 'Custom',
            custom_recognition: 'MyRec',
            action: 'Custom',
            custom_action: 'MyAct',
            custom_action_param: '哈哈哈(*´▽｀)ノノ'
        }
    }

    let detail = await tasker.post_task('Entry', ppover).wait().get()
    if (!detail) {
        console.log('pipeline failed')
        process.exit(1)
    }
    console.log('pipeline detail:', detail)

    tasker.resource?.post_bundle('/path/to/resource')
    tasker.clear_cache()
    const inited = tasker.inited
    const running = tasker.running

    maa.Global.save_draw = true
    maa.Global.stdout_level = 'All'
    maa.Global.log_dir = '.'

    const devices = await maa.AdbController.find()
    console.log('devices', devices)
    const desktop = await maa.Win32Controller.find()
    console.log('desktop', desktop)

    if (!analyzed || !runned) {
        console.log('failed to run custom recognition or action')
        process.exit(1)
    }
}

async function custom_ctrl_test() {
    console.log('test_custom_controller')

    const myCtrl = new MyController()

    const ctrl = new maa.CustomController(
        myCtrl
        // you also can directly extends, implements and instants
        // new (class MyController extends maa.CustomControllerActorDefaultImpl {
        //     ...
        // })()
    )
    let ret = await ctrl.post_connection().wait().succeeded
    const uuid = ctrl.uuid
    ret &&= await ctrl.post_start_app('custom_aaa').wait().succeeded
    ret &&= await ctrl.post_stop_app('custom_bbb').wait().succeeded
    const image_job = await ctrl.post_screencap().wait()
    ret &&= image_job.succeeded
    console.log('image', image_job.get())
    ret &&= await ctrl.post_click(100, 200).wait().succeeded
    ret &&= await ctrl.post_swipe(100, 200, 300, 400, 200).wait().succeeded
    ret &&= await ctrl.post_touch_down(1, 100, 100, 0).wait().succeeded
    ret &&= await ctrl.post_touch_move(1, 200, 200, 0).wait().succeeded
    ret &&= await ctrl.post_touch_up(1).wait().succeeded
    ret &&= await ctrl.post_click_key(32).wait().succeeded
    ret &&= await ctrl.post_input_text('Hello World!').wait().succeeded

    console.log('controller count', myCtrl.count, 'ret', ret)

    // if (myCtrl.count !== 11 || !ret) {
    //     console.log('failed to run custom controller')
    //     process.exit(1)
    // }
}

class MyController implements maa.CustomControllerActor {
    count = 0

    connect(): maa.MaybePromise<boolean> {
        console.log('on MyController.connect')
        this.count += 1
        return true
    }
    connected(): maa.MaybePromise<boolean> {
        console.log('on MyController.connected')
        return true
    }
    request_uuid(): maa.MaybePromise<string | null> {
        console.log('on MyController.request_uuid')
        // this.count += 1
        return '12345678'
    }
    start_app(intent: string): maa.MaybePromise<boolean> {
        console.log('on MyController.start_app, intent', intent)
        this.count += 1
        return true
    }
    stop_app(intent: string): maa.MaybePromise<boolean> {
        console.log('on MyController.stop_app, intent', intent)
        this.count += 1
        return true
    }
    screencap(): maa.MaybePromise<maa.ImageData | null> {
        console.log('on MyController.screencap')
        this.count += 1
        return fs.readFile('empty.png').then(x => x.buffer)
    }
    click(x: number, y: number): maa.MaybePromise<boolean> {
        console.log('on MyController.click, x', x, 'y', y)
        this.count += 1
        return true
    }
    swipe(
        x1: number,
        y1: number,
        x2: number,
        y2: number,
        duration: number
    ): maa.MaybePromise<boolean> {
        console.log(
            'on MyController.swipe, x1',
            x1,
            'y1',
            y1,
            'x2',
            x2,
            'y2',
            y2,
            'duration',
            duration
        )
        this.count += 1
        return true
    }
    touch_down(contact: number, x: number, y: number, pressure: number): maa.MaybePromise<boolean> {
        console.log(
            'on MyController.touch_down, contact',
            contact,
            'x',
            x,
            'y',
            y,
            'pressure',
            pressure
        )
        this.count += 1
        return true
    }
    touch_move(contact: number, x: number, y: number, pressure: number): maa.MaybePromise<boolean> {
        console.log(
            'on MyController.touch_move, contact',
            contact,
            'x',
            x,
            'y',
            y,
            'pressure',
            pressure
        )
        this.count += 1
        return true
    }
    touch_up(contact: number): maa.MaybePromise<boolean> {
        console.log('on MyController.touch_up, contact', contact)
        this.count += 1
        return true
    }
    click_key(keycode: number): maa.MaybePromise<boolean> {
        console.log('on MyController.click_key, keycode', keycode)
        this.count += 1
        return true
    }
    input_text(text: string): maa.MaybePromise<boolean> {
        console.log('on MyController.input_text, text', text)
        this.count += 1
        return true
    }
    key_down(keycode: number): maa.MaybePromise<boolean> {
        console.log('on MyController.key_down, keycode', keycode)
        this.count += 1
        return true
    }
    key_up(keycode: number): maa.MaybePromise<boolean> {
        console.log('on MyController.key_up, keycode', keycode)
        this.count += 1
        return true
    }
}

async function main() {
    console.log('MaaFw Version:', maa.Global.version)
    // console.log('MaaFw Role', maa.AgentRole)

    maa.Global.config_init_option('../../install/bin')

    await api_test()
    await custom_ctrl_test()

    process.exit(0)
}

main()

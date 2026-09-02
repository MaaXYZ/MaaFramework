import './maa-server.ts'
import { expectRegistrationError } from './custom-registration.ts'

const myReco: maa.CustomRecognitionCallback = () => [[0, 0, 0, 0], 'NodeServerRec']
const myAct: maa.CustomActionCallback = () => true

const myReco: maa.CustomRecognitionCallback = () => [[0, 0, 0, 0], 'NodeServerRec']
const myAct: maa.CustomActionCallback = () => true

function expectRegistrationError(register: () => void, expectedMessage: string) {
    let errorMessage = ''
    try {
        register()
    } catch (error) {
        errorMessage = error instanceof Error ? error.message : String(error)
    }
    if (!errorMessage.includes(expectedMessage)) {
        throw new Error(`unexpected custom registration error: ${errorMessage}`)
    }
}

async function main() {
    console.log('MaaFw Version:', maa.Global)
    // console.log('MaaFw Role', maa.AgentRole)

    console.log('AgentServer', maa.Server)

    maa.Server.register_custom_action('NodeServerAct', myAct)
    maa.Server.register_custom_recognition('NodeServerRec', myReco)

    for (const { register, expectedMessage } of [
        {
            register: () => maa.Server.register_custom_action('NodeServerAct', myAct),
            expectedMessage: "Custom name is already registered: 'NodeServerAct'"
        },
        {
            register: () => maa.Server.register_custom_recognition('NodeServerRec', myReco),
            expectedMessage: "Custom name is already registered: 'NodeServerRec'"
        },
        {
            register: () => maa.Server.register_custom_action('NodeServerRec', myAct),
            expectedMessage: "Custom name is already registered: 'NodeServerRec'"
        },
        {
            register: () => maa.Server.register_custom_recognition('NodeServerAct', myReco),
            expectedMessage: "Custom name is already registered: 'NodeServerAct'"
        },
        {
            register: () => maa.Server.register_custom_action('', myAct),
            expectedMessage: 'Custom name must not be empty'
        },
        {
            register: () => maa.Server.register_custom_recognition('', myReco),
            expectedMessage: 'Custom name must not be empty'
        }
    ]) {
        expectRegistrationError(register, expectedMessage)
    }

    maa.Server.register_custom_recognition('NodeServerCaseSensitive', myReco)
    maa.Server.register_custom_action('nodeservercasesensitive', myAct)

    process.exit(0)
}

main()

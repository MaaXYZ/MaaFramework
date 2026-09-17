export function expectRegistrationError(register: () => void, expectedMessage: string) {
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

import path from 'path'
import cp from 'child_process'
import fs from 'fs/promises'

const assetsPath = process.argv[2]
const version = process.argv[3].replace(/^v/, '')

const bindingPath = path.resolve(process.cwd(), path.dirname(process.argv[1]), '..')
const releasePath = path.join(bindingPath, 'release')
// const rootPath = path.resolve(bindingPath, '..', '..', '..')

const platforms: [maa: string, node: string][] = [
    ['win-x86_64', 'win32-x64'],
    ['linux-x86_64', 'linux-x64'],
    ['macos-x86_64', 'darwin-x64'],
    ['win-aarch64', 'win32-arm64'],
    ['linux-aarch64', 'linux-arm64'],
    ['macos-aarch64', 'darwin-arm64'],
]

async function main() {
    const corePackPath = path.join(releasePath, 'maa-node')
    cp.execSync(`npm --prefix ${corePackPath} pkg set version=${version}`)
    for (const dir of ['minitouch', 'maatouch', 'minicap']) {
        await fs.cp(
            path.join(assetsPath, 'MAA-win-x86_64', 'share', 'MaaAgentBinary', dir),
            path.join(corePackPath, 'agent', dir),
            {
                recursive: true,
            },
        )
    }

    for (const [mplat, nplat] of platforms) {
        const maaBinPath = path.join(assetsPath, `MAA-${mplat}`, 'bin')
        const sepPackPath = path.join(releasePath, `maa-node-${nplat}`)

        for (const file of await fs.readdir(maaBinPath)) {
            if (/MaaPiCli/.test(file)) {
                continue
            }
            await fs.cp(path.join(maaBinPath, file), path.join(sepPackPath, file), {
                recursive: true,
            })
        }

        cp.execSync(
            `npm --prefix ${corePackPath} pkg set optionalDependencies.@maaxyz/maa-node-${nplat}=${version}`,
        )
        cp.execSync(`npm --prefix ${sepPackPath} pkg set version=${version}`)
    }
}

main()

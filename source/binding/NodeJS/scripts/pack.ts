import path from 'path'
import cp from 'child_process'
import fs from 'fs/promises'

const assetsPath = process.argv[2]
const version = process.argv[3].replace(/^v/, '')

const bindingPath = path.resolve(process.cwd(), path.dirname(process.argv[1]), '..')
const releasePath = path.join(bindingPath, 'release')
// const rootPath = path.resolve(bindingPath, '..', '..', '..')

const platforms: [maa: string, node: string][] = [
    ['win', 'win32'],
    ['linux', 'linux'],
    ['macos', 'darwin']
]

const archs: [maa: string, node: string][] = [
    ['x86_64', 'x64'],
    ['aarch64', 'arm64']
]

async function main() {
    const corePackPath = path.join(releasePath, 'maa-node')
    cp.execSync(`npm --prefix ${corePackPath} pkg set version=${version}`)
    await fs.rm(path.join(corePackPath, 'node_modules'), { recursive: true })
    for (const dir of ['minitouch', 'maatouch', 'minicap']) {
        await fs.cp(
            path.join(assetsPath, 'MAA-win-x86_64', 'share', 'MaaAgentBinary', dir),
            path.join(corePackPath, 'agent', dir),
            {
                recursive: true
            }
        )
    }

    for (const [mplat, nplat] of platforms) {
        for (const [march, narch] of archs) {
            const maaBinPath = path.join(assetsPath, `MAA-${mplat}-${march}`, 'bin')
            const sepPackPath = path.join(releasePath, `maa-node-${nplat}-${narch}`)

            for (const file of await fs.readdir(maaBinPath)) {
                if (/MaaPiCli/.test(file)) {
                    continue
                }
                await fs.copyFile(path.join(maaBinPath, file), path.join(sepPackPath, file))
            }

            cp.execSync(
                `npm --prefix ${corePackPath} pkg set optionalDependencies.@maaxyz/maa-node-${nplat}-${narch}=${version}`
            )
            cp.execSync(`npm --prefix ${sepPackPath} pkg set version=${version}`)
        }
    }
}

main()

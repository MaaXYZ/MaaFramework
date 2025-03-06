import { build } from 'esbuild'
import { copyFileSync, mkdirSync } from 'fs'

build({
    entryPoints: ['src/index-client.ts', 'src/index-server.ts'],
    outdir: 'dist',
    platform: 'node',
    external: [
        '@maaxyz/maa-node-win32-x64',
        // '@maaxyz/maa-node-win32-arm64',
        '@maaxyz/maa-node-linux-x64',
        '@maaxyz/maa-node-linux-arm64',
        '@maaxyz/maa-node-darwin-x64',
        '@maaxyz/maa-node-darwin-arm64'
    ],
    bundle: true
})

mkdirSync('dist', { recursive: true })
copyFileSync('src/maa.d.ts', 'dist/maa.d.ts')

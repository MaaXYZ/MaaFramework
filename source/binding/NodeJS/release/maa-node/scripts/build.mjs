import { build } from 'esbuild'
import { copyFileSync, mkdirSync } from 'fs'

build({
    entryPoints: ['src/index.ts'],
    outdir: 'dist',
    platform: 'node',
    external: [
        '@nekosu/maa-node-win32-x64',
        '@nekosu/maa-node-win32-arm64',
        '@nekosu/maa-node-linux-x64',
        '@nekosu/maa-node-linux-arm64',
        '@nekosu/maa-node-darwin-x64',
        '@nekosu/maa-node-darwin-arm64'
    ],
    bundle: true
})

mkdirSync('dist', { recursive: true })
copyFileSync('src/maa.d.ts', 'dist/maa.d.ts')

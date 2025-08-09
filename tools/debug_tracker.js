const dgram = require('dgram')
const net = require('net')

const BROADCAST_PORT = 13111
const BROADCAST_ADDR = '255.255.255.255'

const udpClient = dgram.createSocket('udp4')

udpClient.bind(() => {
    udpClient.setBroadcast(true)

    const message = Buffer.from('MaaFramework.DiscoverDebugServer')

    udpClient.send(message, 0, message.length, BROADCAST_PORT, BROADCAST_ADDR, err => {
        if (err) {
            console.error('UDP broadcast send error:', err)
            udpClient.close()
            return
        }
        console.log('UDP broadcast message sent')
    })
})

udpClient.on('message', (msg, rinfo) => {
    const tcpPort = parseInt(msg.toString())
    if (isNaN(tcpPort)) {
        console.log('Received invalid TCP port:', msg.toString())
        return
    }
    console.log(`Received TCP port ${tcpPort}`)

    udpClient.close()

    connectTcp('127.0.0.1', tcpPort)
})

function connectTcp(host, port) {
    const client = new net.Socket()

    client.connect(port, host, () => {
        console.log(`Connected to TCP server at ${host}:${port}`)
    })

    client.on('data', data => {
        const obj = JSON.parse(data.toString())
        console.log('Received from TCP server:', obj)
        client.write('{}\n')
    })

    client.on('close', () => {
        console.log('TCP connection closed')
    })

    client.on('error', err => {
        console.error('TCP connection error:', err)
    })
}

const dgram = require('dgram')
const { WebSocketServer, OPEN } = require('ws');
const rxjs = require('rxjs');
const { spawn } = require('child_process');
const Joi = require('joi');

let proc;

const setServerStatusJoi = Joi.object({
  message_type: Joi.string().equal('set_server_status').required(),
  data: Joi.object({
    server_id: Joi.number().required(),
    stopped: Joi.number().valid(0, 1),
  }).required()
});

const dataUpdateJoi = Joi.object({
  message_type: Joi.string().equal('data_update').required(),
  data: Joi.object({
    server_id: Joi.number().required(),
    index: Joi.number().required().min(0).max(5),
    value: Joi.number().required()
  })
});

const restartJoi = Joi.object({
  message_type: Joi.string().equal('restart').required()
})

const serverMessageJoi = Joi.alternatives().try(
  setServerStatusJoi,
  dataUpdateJoi,
  restartJoi,
)

const { Subject } = rxjs;

const messageSubject = new Subject();

const wss = new WebSocketServer({
  port: 8001,
  perMessageDeflate: {
    zlibDeflateOptions: {
      // See zlib defaults.
      chunkSize: 1024,
      memLevel: 7,
      level: 3
    },
    zlibInflateOptions: {
      chunkSize: 10 * 1024
    },
    // Other options settable:
    clientNoContextTakeover: true, // Defaults to negotiated value.
    serverNoContextTakeover: true, // Defaults to negotiated value.
    serverMaxWindowBits: 10, // Defaults to negotiated value.
    // Below options specified as default values.
    concurrencyLimit: 10, // Limits zlib concurrency for perf.
    threshold: 1024 // Size (in bytes) below which messages
    // should not be compressed if context takeover is disabled.
  }
});

const recvServer = dgram.createSocket({ type: 'udp4', reuseAddr: true });
const sendServer = dgram.createSocket('udp4');
const shouldRunManager = !process.argv.find((it) => it === '--no-manager');

const client_ip = '127.0.0.1';
const client_receive_port = 12344;
const client_send_port = 12346;

recvServer.on('error', (err) => {
  console.log(`server error:\n${err.stack}`);
  recvServer.close();
});

messageSubject.asObservable().subscribe(function (message) {
  wss.clients.forEach(function each(client) {
    if (client.readyState === OPEN) {
      client.send(message, { binary: false });
    }
  });
});

wss.on('connection', function connection(ws) {
  ws.on('message', function message(data) {
    const dataString = data.toString();
    const dataJson = JSON.parse(dataString);
    const validationResult = serverMessageJoi.validate(dataJson);
    if (validationResult.error) {
      return;
    }
    const { value } = validationResult;
    if (value.message_type === 'restart') {
      console.log("Restarting Raft Servers...");
      if (shouldRunManager) {
        proc.kill();

        proc = spawn("../Raft_Implementation/manager");  
      }
      return;
    }
    sendServer.send(JSON.stringify(value), client_send_port, client_ip);
  });
});

recvServer.on('message', (msg, rinfo) => {
  messageSubject.next(msg)
});

recvServer.on('listening', () => {
  const address = recvServer.address();
  console.log(`server listening ${address.address}:${address.port}`);
});

async function main() {  
  if (shouldRunManager) {
    console.log("Building C++ Raft implementation...");

    const make_proc = spawn("make", {
      cwd: "../Raft_Implementation/"
    });

    make_proc.stdout.on('data', (data) => {
      console.log(`stdout: ${data}`);
    });

    await new Promise((resolve) => make_proc.on('close', resolve));

    console.log("Running manager...")

    proc = spawn("../Raft_Implementation/manager");

    proc.stdout.on('data', (data) => {
      console.log(`stdout: ${data}`);
    });

    proc.stderr.on('data', (data) => {
      console.log(`stderr: ${data}`);
    });
  }

  setTimeout(() => {
    console.log(`Binding receive server to port ${client_receive_port}...`);
    recvServer.bind({
      address: client_ip,
      port: client_receive_port,
    });
  }, 2000)

  process.on('SIGINT', function() {
    console.log("Caught interrupt signal");
    if (shouldRunManager) {
      proc.kill();
    }
    wss.close();  
  
    process.exit();
  });  


  if (process.platform === "win32") {
    var rl = require("readline").createInterface({
      input: process.stdin,
      output: process.stdout
    });

    rl.on("SIGINT", function () {
      process.emit("SIGINT");
    });
  }
}

main();

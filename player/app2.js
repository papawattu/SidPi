var fs = require('fs');


var Sid = {
    fd : null,
    openSid: () => {
        return new Promise((resolve,reject) => {
            var fd = fs.open('/dev/sid','w', (err, fd) => {
                    if(err) {
                        reject(Error('File open error ' + err));
                    } else {
                        this.fd = fd;
                        resolve(fd);
                    }
            });
        });
    },
    writeSid: (reg,value) => {
        return new Promise((resolve,reject) => {
            var buf = new Buffer(4);
            buf[0] = reg & 0x1f;
            buf[1] = value & 0xff;
            buf[2] = 0x0;
            buf[3] = 0x0;
            fs.write(this.fd,buf,0,4,(err,written,str) => {
                if(err) {
                    reject(Error(err));
                } else {
                    resolve(written);
                }
            });
        });
    },
    closeSid: () => {
        console.log('Closing Sid device.');
        fs.close(this.fd);
    }
}


Sid.openSid().then((fd) => {
        console.log('Opened Sid device. ' + fd);
        Sid.writeSid(0,255).then((bytes) => {
            console.log('Written ' + bytes + ' bytes.');
            Sid.closeSid();},(err) => { console.log(error);});
        }, (error) => {
            console.log(error);
    });


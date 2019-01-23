package main
 
import (
    "fmt"
    "net"
    "encoding/binary"
    "time"
)

const channelBufSize = 20

type Position struct {
  x, y, z, counter int
}

type Delta struct {
  xDiff, yDiff, zDiff int
  boardAddr int16
}

func Abs(x int) int {
    if x < 0 {
        return -x
    }
    return x
}

func LEDOrch(recieved chan Delta) {
	for {
		delta := <-recieved
		if delta.boardAddr == -1 {
		fmt.Println("no activity")
		} else {
			fmt.Println("some activity", delta)
		}
	}
}

func main() {
    ServerAddr,err := net.ResolveUDPAddr("udp","192.168.1.28:5002")
    _=err
    ServerConn, err := net.ListenUDP("udp", ServerAddr)
    defer ServerConn.Close()
    
    const totalStrands = 20
    const callibrationTimeOut = 10
    const movementTolarance = 180

    buf := make([]byte, 8)
    defaultState := make([]Position, totalStrands)
    start := time.Now()
    //Begin: Start callibrating accelerometer values for 10 seconds
    elapsed := time.Since(start)
    for {
        n,addr,err := ServerConn.ReadFromUDP(buf)
        boardAddr := int16(binary.LittleEndian.Uint16(buf[0:]))% totalStrands
        
        defaultState[boardAddr].x += int(int16(binary.LittleEndian.Uint16(buf[2:])))
        defaultState[boardAddr].y += int(int16(binary.LittleEndian.Uint16(buf[4:])))
        defaultState[boardAddr].z += int(int16(binary.LittleEndian.Uint16(buf[6:])))
        defaultState[boardAddr].counter++
        //fmt.Println("Received addr, ",boardAddr, " ,x, ", defaultState[boardAddr].x , " y ", defaultState[boardAddr].y, " z ", defaultState[boardAddr].z , "Counter" , defaultState[boardAddr].counter)
        if err != nil {
            fmt.Println("Error: ",err)
        } 
        _=n
        _=addr
        _=err
        elapsed = time.Since(start)
             if elapsed.Seconds() > callibrationTimeOut{
				break
        }
    }

    for i := 0; i < totalStrands; i++{
        defaultState[i].x = int(float32(defaultState[i].x) / float32(defaultState[i].counter))
        defaultState[i].y = int(float32(defaultState[i].y) / float32(defaultState[i].counter))
        defaultState[i].z = int(float32(defaultState[i].z) / float32(defaultState[i].counter))
    }
    fmt.Println("Accelerometer callibrated")
	//End Averaging
	
	send := make(chan Delta, channelBufSize)//Data struct to send to the thread
	go LEDOrch(send)
	var temp Delta
    for {
        n,addr,err := ServerConn.ReadFromUDP(buf)
        boardAddr := int16(binary.LittleEndian.Uint16(buf[0:]))% totalStrands
        
        xDiff := int(int16(binary.LittleEndian.Uint16(buf[2:]))) - defaultState[boardAddr].x
        yDiff := int(int16(binary.LittleEndian.Uint16(buf[4:]))) - defaultState[boardAddr].y
        zDiff := int(int16(binary.LittleEndian.Uint16(buf[6:]))) - defaultState[boardAddr].z
        if ((Abs(xDiff) > movementTolarance) || (Abs(yDiff) > movementTolarance) || (Abs(zDiff) > movementTolarance)){
			temp.boardAddr = boardAddr
			temp.xDiff = xDiff
			temp.yDiff = yDiff
			temp.zDiff = zDiff
			fmt.Println("sending something")
		} else {
			temp.boardAddr = -1
			temp.xDiff = 0
			temp.yDiff = 0
			temp.zDiff = 0
		}
		send <- temp
        //fmt.Println("Received addr, ",boardAddr, " ,x, ", defaultState[boardAddr].x , " y ", defaultState[boardAddr].y, " z ", defaultState[boardAddr].z , "Counter" , defaultState[boardAddr].counter)
        if err != nil {
            fmt.Println("Error: ",err)
        } 
        _=n
        _=addr
        _=err
    }
    
}
package main
 
import (
    "fmt"
    "net"
    "encoding/binary"
    "time"
)
 
type Position struct {
  x, y, z, counter int
}

func Abs(x int) int {
    if x < 0 {
        return -x
    }
    return x
}

func main() {
    ServerAddr,err := net.ResolveUDPAddr("udp","192.168.1.28:5002")
    _=err
    ServerConn, err := net.ListenUDP("udp", ServerAddr)
    defer ServerConn.Close()
    
    const totalStrands = 20
    const callibrationTimeOut = 10
    const movementTolarance = 100

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
    for {
        n,addr,err := ServerConn.ReadFromUDP(buf)
        boardAddr := int16(binary.LittleEndian.Uint16(buf[0:]))% totalStrands
        
        xDiff := Abs(int(int16(binary.LittleEndian.Uint16(buf[2:]))) - defaultState[boardAddr].x)
        if xDiff > movementTolarance {
            fmt.Println("Board ", boardAddr, "moved in x dir!!", xDiff)
        }
        yDiff := Abs(int(int16(binary.LittleEndian.Uint16(buf[4:]))) - defaultState[boardAddr].y)
        if yDiff > movementTolarance {
            fmt.Println("Board ", boardAddr, "moved in y dir!!", yDiff)
        }
        zDiff := Abs(int(int16(binary.LittleEndian.Uint16(buf[6:]))) - defaultState[boardAddr].z)
        if zDiff > movementTolarance {
            fmt.Println("Board ", boardAddr, "moved in z dir!!", zDiff)
        }
        defaultState[boardAddr].counter++
        //fmt.Println("Received addr, ",boardAddr, " ,x, ", defaultState[boardAddr].x , " y ", defaultState[boardAddr].y, " z ", defaultState[boardAddr].z , "Counter" , defaultState[boardAddr].counter)
        if err != nil {
            fmt.Println("Error: ",err)
        } 
        _=n
        _=addr
        _=err
    }
    
}
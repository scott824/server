/*
 * Automatically generated by jrpcgen 0.95.1 on 12/18/01 7:23 PM
 * jrpcgen is part of the "Remote Tea" ONC/RPC package for Java
 * See http://acplt.org/ks/remotetea.html for details
 */
package com.sleepycat.db.rpcserver;
import org.acplt.oncrpc.*;
import java.io.IOException;

public class __env_flags_msg implements XdrAble {
    public int dbenvcl_id;
    public int flags;
    public int onoff;

    public __env_flags_msg() {
    }

    public __env_flags_msg(XdrDecodingStream xdr)
           throws OncRpcException, IOException {
        xdrDecode(xdr);
    }

    public void xdrEncode(XdrEncodingStream xdr)
           throws OncRpcException, IOException {
        xdr.xdrEncodeInt(dbenvcl_id);
        xdr.xdrEncodeInt(flags);
        xdr.xdrEncodeInt(onoff);
    }

    public void xdrDecode(XdrDecodingStream xdr)
           throws OncRpcException, IOException {
        dbenvcl_id = xdr.xdrDecodeInt();
        flags = xdr.xdrDecodeInt();
        onoff = xdr.xdrDecodeInt();
    }

}
// End of __env_flags_msg.java

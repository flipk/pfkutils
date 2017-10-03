/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MAGIC_H_
#define __MAGIC_H_

// this file lists all magic numbers in use in the system.
// this way the assignment of random numbers is not haphazard
// and guarantees uniqueness.

enum MagicNumbers {

    MagicNumbers_Encrypt_KeyNone                = 0x7456e5c5,
    MagicNumbers_Encrypt_ClassNone              = 0x74aacf23,
    MagicNumbers_Encrypt_KeyRubik5              = 0x10a7fe59,
    MagicNumbers_Encrypt_ClassRubik5            = 0x68adf434,
    MagicNumbers_Encrypt_KeyRubik4              = 0x7dd744b7,
    MagicNumbers_Encrypt_ClassRubik4            = 0x3b927dca,

    MagicNumbers_MessageEncoding_Header         = 0x31bc397f,
    MagicNumbers_MessageEncoding_HeaderEnc      = 0x126735df,
    MagicNumbers_MessageEncoding_GenericType    = 0x095be460,

    MagicNumbers_Messages_MessageFdActive       = 0x08553203,
    MagicNumbers_Messages_MsgsTcpIndication     = 0x48492709,
    MagicNumbers_Messages_ConnEst               = 0x856491df,
    MagicNumbers_Messages_KillConn              = 0x3ebb51d7,
    MagicNumbers_Messages_NewKeyReq             = 0x69098f33,
    MagicNumbers_Messages_NewKeyAck             = 0xd7a30ea5,
    MagicNumbers_Messages_NewKeyReqInt          = 0x3a23685d,
    MagicNumbers_Messages_NewKeyAckInt          = 0x1fe16547,
    MagicNumbers_Messages_LookupReq             = 0xbe788abf,
    MagicNumbers_Messages_LookupReply           = 0xbb28b0f3,
    MagicNumbers_Messages_conn_ind              = 0x094a699a,
    MagicNumbers_Messages_discon_ind            = 0x26f33c08,

    MagicNumbers_Threads_PrintMessage           = 0x24fec2ea

};

#endif /* __MAGIC_H_ */

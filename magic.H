/*
 * This code is originally written by Phillip F Knaack.
 * There are absolutely no restrictions on the use of this code.  It is
 * NOT GPL!  If you use this code in a product, you do NOT have to release
 * your alterations, nor do you have to acknowledge in any way that you
 * are using this software.
 * The only thing I ask is this: do not claim you wrote it.  My name
 * should appear in this file forever as the original author, and every
 * person who modifies this file after me should also place their names
 * in this file (so those that follow know who broke what).
 * This restriction is not enforced in any way by any license terms, it
 * is merely a personal request from me to you.  If you wanted, you could
 * even completely remove this entire comment and place a new one with your
 * company's confidential proprietary license on it-- but if you are a good
 * internet citizen, you will comply with my request out of the goodness
 * of your heart.
 * If you do use this code and you make a buttload of money off of it,
 * I would appreciate a little kickback-- but again this is a personal
 * request and is not required by any licensing of this code.  (Of course
 * in the offchance that anyone every actually DOES make a lot of money 
 * using this code, I know I'm going to regret that statement, but at
 * this point in time it feels like the right thing to say.)
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

#if 0
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
#endif

};

#endif /* __MAGIC_H_ */

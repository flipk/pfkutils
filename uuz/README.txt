
UUZ is a tool for compressing and encoding binary files in ASCII
text. Only basic printable characters from the 7-bit ASCII character
set are used.  The name is derived from the original Unix programs
"uuencode" and "uucp", and the "z" from the "zlib" compression library
used in uuz.  uuencode and uucp were used to transport 8-bit binary
files from unix computer to unix computer over "7-bit dirty" networks
by encoding them in ASCII character sets.

Indeed, the original base-64 character set used by uuencode is
supported by uuz, although the output of this program is in no way
whatsoever compatible with uuencode/uudecode.

UUZ produces text files containing "S records". Note that while this
idea was inspired by the S-record file format invented by Motorola in
the 1970's (also known as SRECORD, SREC, S19, S28, S37), it is
actually almost nothing like it.

The (optional) compression used is libz, at its maximum setting,
roughly equivalent to "gzip -9".

The (optional) encryption used is AES256 in CBC mode, as supplied by
the mbedtls crypto library.  The key for AES256 is generated using
SHA256 of the supplied password (TODO: use a more formal KDF method).
The IV for CBC mode is randomly generated for every file.  When
encryption is enabled, SHA-256-HMAC is also used as well; HMAC is done
in blocks of roughly 4 kilobytes.  The key for each HMAC is constructed by
concatenating the following values:
  - the encryption key;
  - the message size as a decimal ascii string;
  - and a random salt value as a decimal ascii string.

UUZ files are text files containing only ASCII characters between 0x20
(space) and 0x7e (tidle), and newlines characters; the encoder creates
newlines as 0x0A, while the decoder accepts 0x0A and/or 0x0D.

The first character of a line is an 'S' or an 's'. Upper vs lower case
is significant: lines beginning with lower case 's' are for first and
intermediate lines of a multi-line record, while the upper case 'S' is
for the final (or only) line of a record.

The second character is a digit, '1', '2', '3', or '9'.  The digit
indicates one of four record types as described below.  The remainder
of the line is encoded depending on the record type.  The maximum length
of a line is 78 characters: [Ss][1239] followed by 76 characters of
encoded data.  The data is encoded using one of the following methods:

 - one binary byte becomes two lower-case hexadecimal digits.
 - base64 : three binary bytes (24 bits) become four 6-bit values
   which then index a character-set of 64 ASCII characters.  The
   "newbase64" library supports 9 different base64 encoding
   'standards' as found across the internet.  (At the time of this
   writing, there are also 6 additional base64 standards documented,
   but not supported, by this library.)

Messages are encoded using protobuf. All messages are of type uuzMsg.
All messages are protobuf-serialized to a binary byte stream.

- If encryption is not enabled, this byte stream is then encoded using
  one of the two above algorithms (hex or one of the base64 variants).

- If encryption is enabled, this byte stream is padded as necessary
  for encryption, encrypted, and then nested into a 'bytes' field of
  an EncryptedContainer protobuf message. This message contains fields
  necessary to support both encryption and integrity algorithms,
  including an initialization vector (iv) for the encryption (if
  required, not all messages require it) and a salt for the HMAC.
  This message is then serialized into a byte stream and encoded using
  hex or base64.

    - NOTE the S1 record type is never compressed, encrypted, or hmac'd.

The encoded message is broken up into 76-character (or less) lines
which are then prepended with the S-code and digit. If it takes more than
one line to encode, all lines except the last start with lower-case 's',
and the final (or only) line starts with an upper case 'S'.

All four S record types encode a uuzMsg protobuf message. The type of S
record corresponds to the value of uuzMsg.type enum.

Here are the four types of S records:

   - S1 records / version block
      - contain uuzMsg with type = uuz_VERSION.
      - "god" record: always the first record, thou shalt not have any
        other S records before it.
      - only one instance of this record.
      - always encoded using hex digits, not base64.
         - this message specifies the base64 variant used
	   for all other S records.
      - never compressed, encrypted, or hmac'd.
         - this message specifies the compression, encryption,
	   and hmac settings for all other S records.
      - because this message is never encrypted, it is also never
        nested in an EncryptedContainer.

(Note, for the other types of S records following, if the S1 record
 indicates encryption is NOT in use, the encoded record is always the
 uuzMsg itself.  If S1 indicates encryption IS in use, the encoded
 record is always encapsulated in an EncryptedContainer message.  The
 decoder decrypts and validates the HMAC for every S record. If the
 HMAC cannot be validated, the decoder reports and error and refuses
 to continue.)

   - s2/S2 record / file info
      - contain uuzMsg with type = uuz_FILE_INFO
      - when an s2/S2 record is encrypted, it also specifies an
	initialization vector (iv) for the encryption. this IV value
	begins a stream in which the IV is updated after every
        encryption unit. This IV continues to update until the next S2
	record is encountered.
      - one instance of this record per encoded file.
      - describes file name, file size, and file mode (chmod).
      - must be after S1 record and before s3/S3 records.
      - the decoder initializes a SHA256 message digest upon processing
        this message.
   - s3/S3 record / file data
      - contains uuzMsg with type = uuz_FILE_DATA
      - when an s3/S3 record is encrypted, the IV is never set,
        because it is assumed to continue to use the updated IV after
	the previous s3/S3 record or the preceding s2/S2 record.
      - this message contains data for the file described in the
        preceding s2/S2 record.
	- if S1 enables compression, this data is actually
	  the compressed data.
      - this message contains the byte-counter for the current
        position in the file; the decoder should count the number of
        bytes in each s3/S3 and count along with this position
        indicator, verifying all data that should be present, is.
        - if S1 enables compression, this position counter indicates
	  the position in the compressed data stream, not the uncompressed
	  data stream.
      - data from these records (or output from the uncompressor if
        compressed) is passed into the SHA256 message digest.
   - s9/S9 record
      - contains uuzMsg with type = uuz_FILE_COMPLETE
      - the decoder finalizes the uncompressor upon decoding this message.
      - contains the final compressed size of the data stream for a file,
        or the uncompressed size if S1 does not enable compression.
	- the decoder will validate the final amount of data input to
	  the uncompressor (or the data file if not compressed) matches
	  this value and will report an error if it does not match.
      - contains a SHA256 hash for this file; the decoder will finalize
        the SHA256 message digest and compare the result.

OVERHEAD

Base64 encoding causes a certain amount of overhead. Because three
bytes becomes four, this is an automatic 33% overhead.  The S record
encoding adds about 4%, and protobuf encoding adds another 2% overhead.

Encryption adds more overhead, because it's a second nested layer of
protobuf, as well as adding an IV per file and a salt and HMAC value
per data message. This can add another 3% to 4%.

Compression offsets these values, and therefore the overhead is
completely dependent on the file data being encoded. For instance on C
code, compression ratios of 5:1 are possible, but files already
compressed will not be further compressed at all, which includes
images (i.e. JPG), movies (i.e. MP4), and anything already compressed
with gzip, bzip, winzip, etc.

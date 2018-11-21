Botan: Crypto and TLS for C++11
========================================

Botan (Japanese for peony) is a cryptography library written in C++11
and released under the permissive `Simplified BSD
<https://botan.randombit.net/license.txt>`_ license.

Botan's goal is to be the best option for cryptography in C++ by offering the
tools necessary to implement a range of practical systems, such as TLS/DTLS,
X.509 certificates, modern AEAD ciphers, PKCS#11 and TPM hardware support,
password hashing, and post quantum crypto schemes. Botan also has a C89 API
specifically designed to be easy to call from other languages. A Python binding
using ctypes is included, and several other `language bindings
<https://github.com/randombit/botan/wiki/Language-Bindings>`_ are available.
Find the full feature list below.

Development is coordinated on `GitHub <https://github.com/randombit/botan>`_
and contributions are welcome (read `doc/contributing.rst` for more info).

If you need help with a problem, please open an `issue on GitHub
<https://github.com/randombit/botan/issues>`_ or email the
`botan-devel mailing list
<https://lists.randombit.net/mailman/listinfo/botan-devel/>`_.

New releases are announced on the
`botan-announce mailing list
<https://lists.randombit.net/mailman/listinfo/botan-announce/>`_.

If you think you have found a security bug in Botan please contact
Jack Lloyd by emailing jack@randombit.net. His PGP public key with
fingerprint 4E60C73551AF2188DF0A5A6278E9804357123B60 can can be found
in ``doc/pgpkey.txt`` in the distribution,
https://keybase.io/jacklloyd, and some public PGP key servers.

.. highlight:: none

For all the details on building the library, read the
`users manual <https://botan.randombit.net/manual>`_, but basically::

  $ ./configure.py
  $ make
  $ ./botan-test
  ...
  $ make install

Botan can also be built into a single-file amalgamation for easy inclusion into
external build systems, see the manual for details.

.. image:: https://travis-ci.org/randombit/botan.svg?branch=master
    :target: https://travis-ci.org/randombit/botan
    :alt: Travis CI status

.. image:: https://ci.appveyor.com/api/projects/status/n9f94dljd03j2lce/branch/master?svg=true
    :target: https://ci.appveyor.com/project/randombit/botan/branch/master
    :alt: AppVeyor CI status

.. image:: https://botan-ci.kullo.net/badge
    :target: https://botan-ci.kullo.net/
    :alt: Kullo CI status

.. image:: https://codecov.io/github/randombit/botan/coverage.svg?branch=master
    :target: https://codecov.io/github/randombit/botan
    :alt: Code coverage report

.. image:: https://scan.coverity.com/projects/624/badge.svg
    :target: https://scan.coverity.com/projects/624
    :alt: Coverity results

.. image:: https://sonarcloud.io/api/project_badges/measure?project=botan&metric=ncloc
    :target: https://sonarcloud.io/dashboard/index/botan
    :alt: Sonarcloud analysis

.. image:: https://bestpractices.coreinfrastructure.org/projects/531/badge
    :target: https://bestpractices.coreinfrastructure.org/projects/531
    :alt: CII Best Practices statement

Release Downloads
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See the `release notes <https://botan.randombit.net/news.html>`_ and
`security advisories <https://botan.randombit.net/security.html>`_

All releases are signed with a
`PGP key <https://botan.randombit.net/pgpkey.txt>`_::

  pub   2048R/EFBADFBC 2004-10-30
        Key fingerprint = 621D AF64 11E1 851C 4CF9  A2E1 6211 EBF1 EFBA DFBC
  uid                  Botan Distribution Key

Some `distributions <https://github.com/randombit/botan/wiki/Distros>`_
such as Arch, Fedora and Debian include packages for Botan. However
these are often out of date; using the latest source release is recommended.

Current Stable Release
----------------------------------------

Version 2 requires a C++11 compiler; GCC 4.8 and later, Clang 3.8 and later, and
MSVC 2015/2017 are regularly tested. New releases of Botan 2 are made on a
quarterly basis.

The latest 2.x release is
`2.7.0 <https://botan.randombit.net/releases/Botan-2.7.0.tgz>`_
`(sig) <https://botan.randombit.net/releases/Botan-2.7.0.tgz.asc>`_
released on 2018-07-02

Old Release
----------------------------------------

The 1.10 branch is the last version of the library written in C++98. It is no
longer supported except for critical security updates (with all support ending
in December 2018), and the developers do not recommend its use anymore.

The latest 1.10 release is
`1.10.17 <https://botan.randombit.net/releases/Botan-1.10.17.tgz>`_
`(sig) <https://botan.randombit.net/releases/Botan-1.10.17.tgz.asc>`_
released on 2017-10-02

Find Enclosed
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Transport Layer Security (TLS) Protocol
----------------------------------------

* TLS v1.0, v1.1, and v1.2. The broken SSLv3 protocol is no longer supported.
* DTLS v1.0 and v1.2 are adaptations of TLS to datagram operation.
* Extensions include session tickets, SNI, ALPN, OCSP staple requests (client
  side only right now), encrypt-then-mac CBC, and extended master secret.
* Supports authentication using preshared keys (PSK) or passwords (SRP)
* Supports record encryption with ChaCha20Poly1305, AES/OCB, AES/GCM, AES/CCM,
  Camellia/GCM as well as legacy CBC ciphersuites.
* Key exchange using CECPQ1, ECDH, FFDHE, or RSA

Public Key Infrastructure
----------------------------------------

* X.509v3 certificates and CRL creation and handling
* PKIX certificate path validation, including name constraints.
* OCSP request creation and response handling
* PKCS #10 certificate request generation and processing
* SQL database backed certificate store

Public Key Cryptography
----------------------------------------

* RSA signatures and encryption
* DH and ECDH key agreement
* Signature schemes ECDSA, DSA, Ed25519, ECGDSA, ECKCDSA, SM2, and GOST 34.10-2001
* Post-quantum signature scheme XMSS
* Post-quantum key agreement schemes McEliece and NewHope
* ElGamal encryption
* Padding schemes OAEP, PSS, PKCS #1 v1.5, X9.31

Ciphers, hashes, MACs, and checksums
----------------------------------------

* Authenticated cipher modes EAX, OCB, GCM, SIV, CCM, and ChaCha20Poly1305
* Cipher modes CTR, CBC, XTS, CFB, and OFB
* Block ciphers AES, ARIA, Blowfish, Camellia, CAST-128, CAST-256,
  DES/3DES, GOST 28147, IDEA, KASUMI, Lion, MISTY1, Noekeon, SEED,
  Serpent, SHACAL2, SM4, Threefish-512, Twofish, XTEA
* Stream ciphers ChaCha20, Salsa20/XSalsa20, SHAKE-128, and RC4
* Hash functions SHA-1, SHA-2, SHA-3, RIPEMD-160, Skein-512,
  BLAKE2b, SM3, Tiger, Whirlpool, GOST 34.11, MD5, MD4
* Hash function combiners Parallel and Comb4P
* Authentication codes HMAC, CMAC, Poly1305, SipHash, GMAC, CBC-MAC, X9.19 DES-MAC
* Non-cryptographic checksums Adler32, CRC24, and CRC32

Other Useful Things
----------------------------------------

* Full C++ PKCS #11 API wrapper
* Interfaces for TPM v1.2 device access
* Simple compression API wrapping zlib, bzip2, and lzma libraries
* RNG wrappers for system RNG and hardware RNGs
* HMAC_DRBG and entropy collection system for userspace RNGs
* Password based key derivation functions PBKDF2 and Scrypt
* Password hashing function bcrypt and passhash9 (custom PBKDF scheme)
* SRP-6a password authenticated key exchange
* Key derivation functions including HKDF, KDF2, SP 800-108, SP 800-56A, SP 800-56C
* HOTP and TOTP algorithms
* Format preserving encryption scheme FE1
* Threshold secret sharing
* NIST key wrapping

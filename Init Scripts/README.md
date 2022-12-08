# Init Scripts Usage

- These init scripts were used in the final project demo to start the server and client binaries on startup.
- The init scripts contain simple delays using sleep functions after which the server and client applications are run.
- These scripts have to be added manually into the buildroot images after they are built. They can also be added as part of a package with specific start scripts being added for the client and server accordingly.
- After adding the init scripts, both the Raspberry Pi's for the client and server have to be booted at the same time for correct usage.
- Additionally, these scripts require the management socket to be up and running prior to boot up.
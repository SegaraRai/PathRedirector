# PathRedirector

## Usage example

1. create `C:\RedirectConfig.yaml` like as follows:

   ```yaml
   rules:
     - type: filepath
       last: true
       source: "%UserProfile%\\Documents"
       destination: ".\\Documents"
       exclude:
         - "desktop.ini"
     - type: filepath
       last: true
       source: "%UserProfile%\\Pictures"
       destination: ".\\Pictures"
   ```

2. set `C:\RedirectConfig.yaml` to environment variable `PATHREDIRECTOR_CONFIG_FILE`

3. run target process with PathRedirector dll injected.

with [InjectExec](https://github.com/SegaraRai/InjectExec):

```bat
set PATHREDIRECTOR_CONFIG_FILE=C:\RedirectConfig.yaml

InjectExec.exe notepad.exe PathRedirector64.dll
```

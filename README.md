# passmngr

The passmngr tool is a wrapper around an encrypted key-value "database". The encrypted key-value database has some arbitrary restrictions that will affect password management. The kv store operates in an empty directory with a path length (including file names) that is less than 1024 B long. Additionaly individual key value pairs cannot exceed 2036 B. 

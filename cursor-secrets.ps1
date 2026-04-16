$token = Read-Host "GitLab token" -AsSecureString
$ptr = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($token)
try {
    $env:GITLAB_TOKEN = [System.Runtime.InteropServices.Marshal]::PtrToStringBSTR($ptr)
    "GITLAB_TOKEN loaded in current shell"
} finally {
    [System.Runtime.InteropServices.Marshal]::ZeroFreeBSTR($ptr)
}
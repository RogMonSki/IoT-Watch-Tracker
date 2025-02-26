# build.ps1
# fire up iot:magic with . mapped as a volume and do a pio build

$location = "$(Get-Location)"
$volumeMapping = "${location}:/home/ubuntu/project"

$commandString = "docker run " +
"-it --user 1000:1000 --group-add=dialout --group-add=ubuntu " +
"--volume=`"$volumeMapping`" hamishcunningham/iot:magic " +
"pio run $args"

Write-Host "running docker build: $commandString"
Invoke-Expression $commandString

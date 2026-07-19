use strict;
use warnings;

my $text = <<'END';
[Server::Production]
# Primary telemetry & cluster routing
endpoint = "https://admin:p%40ssw0rd@api.cluster-01.us-east.prod.internal:8443/v2/telemetry?depth=full&retry=true"
matrix = ((A + B) * (C - (D / E)))
owner_email = "sysadmin.core+alerts-2026@sub-domain.enterprise-cloud.org"
data_packet = <[{ID: 0x7F9A, Status: "ACTIVE", Tags:[alpha, beta, gamma]}]>
END

my $pat = <<'END_PAT';
(?xm)
(?<=endpoint\s=\s")
(?&URI)
(?="$)
(?(DEFINE)
  (?<URI>      (?&SCHEME) :// (?:(?&USERINFO) @)? (?&HOST) (?: : (?&PORT) )? (?&PATH) (?: \? (?&QUERY) )? )
  (?<SCHEME>   https? | sftp )
  (?<USERINFO> (?> [a-zA-Z0-9._%+-]++ ) (?: : (?> [a-zA-Z0-9._%+-]++ ) )? )
  (?<HOST>     (?: (?&SUBDOMAIN) \. )++ (?&TLD) )
  (?<SUBDOMAIN>[a-zA-Z0-9](?>[a-zA-Z0-9-]*[a-zA-Z0-9])?)
  (?<TLD>      (?:[a-zA-Z]{2,}|internal|prod|local))
  (?<PORT>     \d{1,5} )
  (?<PATH>     (?: / [a-zA-Z0-9._-]+ )* )
  (?<QUERY>    (?&PARAM) (?: & (?&PARAM) )* )
  (?<PARAM>    [a-zA-Z0-9_]+ = [a-zA-Z0-9_.-]+ )
)
END_PAT

if ($text =~ /$pat/) {
    print "Match!\n";
} else {
    print "No match!\n";
}

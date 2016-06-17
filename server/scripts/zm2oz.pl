#!/usr/bin/perl
# Temporary checkin - I'll remove this once I'm sure all name replacements
# are done

# Place this at the root of the directory you want to replace, run and pray

use File::Find::Rule;
use File::Basename;

my @files = File::Find::Rule->file->name('*.c','*.cpp','*.h','*.am', '*.ac', '*.in')->in('.');
foreach $if (@files)
{
	print "Processing $if..";
	my ($f, $d, $s) = fileparse($if);
	my $of = $if;
	if ($f =~ /^zm/)
	{
		my $nf = substr $f,2;
		$of = $d.'oz'.$nf.$s;
		print "replace with $of";
	}
	
	open my $handle, '<', $if;
	chomp(my @lines = <$handle>);
	close $handle;

	foreach $l (@lines)
	{
		$l =~ s/zm/oz/g;
		$l =~ s/ZM_/OZ_/g;

	}
	
	unlink ($if);
	open my $handle, ">",$of;
	print $handle "$_\n" for @lines;
	close ($handle);

	print "\n";
}


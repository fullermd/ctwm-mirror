#!/usr/bin/env perl
use strict;
use warnings;

# Slurp
MAINLOOP: while(<STDIN>)
{
	# Do basic processing of each line first
	$_ = ilcvt($_);


	# Custom .EX and .EE macros for defining examples
	if(/^\.EX/)
	{
		# Replace this line with start of an example block
		print "------\n";

		# Read lines and output until the end
		while((my $ln = <STDIN>) !~ /^\.EE/)
		{
			# Get rid of \fWhatever manipulations
			$ln =~ s,\\f[IBP],,g;

			print $ln;
		};

		# And close
		print "------\n";

		# Back around to the top
		next MAINLOOP;
	}


	# Now std man macros
	# section headers -> lev1 headers
	s,^\.SH "?([^"]+)"?,\n== $1,;

	# Paragraphs are easy, they're just blank lines
	s,^.PP$,,;


	# These .TP's are a little tougher.  They basically turn into
	# labelled lists, in asciidoc parlance.
	if(/^\.TP/)
	{
		# Skip the .TP line totally.
		print "\n";

		# Next line is the label
		chomp(my $lbl = <STDIN>);

		# Clean it up.  First translate away the .B altogether, and the
		# optional quotes.
		$lbl =~ s,^.B "?([^"]+)"?,$1,;

		# A few have \f[ont] stuff
		$lbl =~ s,\\f[IP],,g;

		# And extra backslashes
		$lbl =~ s,\\,,g;

		print "${lbl}::\n";


		# Now we have lines of content.  Assume it ends as soon as we hit
		# another macro at the start.
		while(<STDIN>)
		{
			redo MAINLOOP if /^\./;

			# Otherwise, do std inline processing, and output
			chomp;
			$_ = ilcvt($_);
			print "  $_\n";
		}

		# NOTREACHED?
		next MAINLOOP;
	}


	print;
}


# Footer
print "\n\n\n// vi" . "m:ft=asciidoc:expandtab:\n";

exit;



# Basic conversion of inline stuff
sub ilcvt
{
	my ($l) = shift;

	# Font conversions
	# \fIsomething\fP makes it underlined.  This seems to be used a lot for
	# commands and filenames and the like, so decide to treat them as
	# monospace.
	# \fB makes bold.  This seems used for an awful lot, but often literal
	# text like config params etc.  That....  seems like it should generally
	# be monospace too, doesn't it?
	$l =~ s,\\f[IBP],++,g;

	# Convert single quoted text
	$l =~ s,\\\(oq,`,g;
	$l =~ s,\\\(cq,',g;

	# Deal with a couple squirrely things asciidoc tries to take as magic
	$l =~ s,\[\+,\\\[+,g;
	$l =~ s,\(\(,\\((,g;

	# Turn all these backslashed hyphens into plain
	$l =~ s,\\-,-,g;

	# This happens a couple times, but asciidoc only cares once?
	#$l =~ s,(\+\+ctwm\+\+)'s,$1\\'s,g;


	return $l;
}

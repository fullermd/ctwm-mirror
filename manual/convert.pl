#!/usr/bin/env perl
use strict;
use warnings;

# Slurp
MAINLOOP: while(<STDIN>)
{
	# Custom .EX and .EE macros for defining examples
	if(/^\.EX/)
	{
		print proc_ex();

		# Back around to the top
		next MAINLOOP;
	}


	# Now std man macros
	# section headers -> lev1 headers
	s,^\.SH "?([^"]+)"?,\n== $1,;

	# Paragraphs are easy, they're just blank lines
	s,^.PP$,,;


	# These .TP's are a little tougher.  They basically turn into
	# labelled lists, in asciidoc parlance.  .IP is another
	# almost-identical way of phrasing it in man (jeez), except
	# incompatible.  Naturally.
	if(/^\.(TP|IP )/)
	{
		# If it's .TP, the following line is the label.  If it's .IP, the
		# label is in the line, probably quotes, with possibly an indent
		# length hanging on the end.  Except when it's not; there are a
		# few cases of /^.IP$/, but I'm intentionally excluding them with
		# the space above.

		# Start with whitespace
		print "\n";

		my $lbl;
		if(/^\.TP/)
		{
			# Next line is the label
			chomp($lbl = <STDIN>);

			# Clean it up.  First translate away the .B altogether, and
			# the optional quotes.
			$lbl =~ s,^.B "?([^"]+)"?,$1,;

			# Do inline translation
			$lbl = ilcvt($lbl);
		}
		elsif(/^\.IP/)
		{
			chomp($lbl = $_);

			# Strip away the .IP leader, and the trailing indent number
			$lbl =~ s,^.IP "?(.+?)"? [48]$,$1,;

			# Some have \f[ont] stuff.  In fact, most have \fB'ing of the
			# label, which we don't want, but many have \fI'ing of args,
			# which we do.  Fortunately, I _think_ we can use spaces to
			# delimiate, because the \fB parts are always the first word.
			$lbl =~ s,\\fB([^\s]+)\\fP,$1,g;

			# The f.setpriority line does weird stuff.
			$lbl =~ s,\\\*,,g if $lbl =~ /^f\.setpriority/;

			# Do remaining inline conversion
			$lbl = ilcvt($lbl);
		}
		# Now we've got just the label itself ready to go.

		# Clean up any space
		$lbl =~ s,(^\s+|\s+$),,;

		print "${lbl}::\n";


		# Now we have lines of content.  Assume it ends as soon as we hit
		# another macro at the start.
		while(<STDIN>)
		{
			# Skip a comment and a weird (ms?) macro that show up before
			# the f.setpriority line.
			next if /^\\"/;
			next if /^\.ds/;

			# We consider this ended if we hit a macro line
			if(/^\./)
			{
				# If it's not another .TP/.IP, add a blank line
				print "\n" unless /^\.(TP|IP)/;
				redo MAINLOOP;
			}

			# Otherwise, do std inline processing, and output
			chomp;
			$_ = ilcvt($_);
			$_ = "  $_\n" if $_;
			print $_;
		}

		# NOTREACHED?
		next MAINLOOP;
	}


	# Do basic inline processing before outputting.
	$_ = ilcvt($_);
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

	# Convery comments
	$l =~ s,^\.\\",// ,;


	return $l;
}


# Process a .EX/.EE block
sub proc_ex
{
	# Assume we've already read the .EX
	my $r;

	# Replace with start of an example block
	$r .= "\n------\n";

	# Read lines and output until the end
	while((my $ln = <STDIN>) !~ /^\.EE/)
	{
		# Get rid of \fWhatever manipulations
		$ln =~ s,\\f[IBP],,g;

		$r .= $ln;
	};

	# And close
	$r .= "------\n\n";


	# Done
	return $r;
}

# r13.asm
# ROT13 cipher implemented in MIPS assembly
.data
INPUT: .ascii
.text
		li $v0,8		# Set syscall to get a string
		la $a0,INPUT		# Set syscall to write to address INPUT
		li $a1,255		# Set syscall to get max 255 characters
		syscall
		li $v0,11		# Set syscall to put a char
		li $t0,0		# String offset
.readloop:	lb $t1,INPUT($t0)	# Read a single byte of the string
		add $t0,$t0,1		# Increment the offset
		beqz $t1,.end		# end if the string is over
		
		# Extract the 6th bit. This bit is set on all alphabetic chars
		srl $t2,$t1,6		
		andi $t2,$t2,1		
		
		# Extract the first 5 bits, which holds the alphabetical number of the char
		andi $t3,$t1,0x1F	# 00011111
		
		# Determine whether the value is alphabetic
		bne $t2,1,.nonalpha	# Not alpha if the 6th bit isn't set
		bgt $t3,26,.nonalpha	# Not alpha if the lowest 5 bits exceed 26
			
		andi $t4,$t1,0x60	# Case bits
		# Rotate the val
		add $t5,$t3,12		
		li $t6,26
		div $t5,$t6
		mfhi $t5
		add $t5,$t5,1
		
		# Re-add the case bits
		or $a0,$t4,$t5
		j .endcond

.nonalpha:	move $a0,$t1		# If the character is non-alphabetic, move to print register
.endcond:	syscall
		j .readloop
.end:		

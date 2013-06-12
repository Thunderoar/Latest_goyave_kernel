#define APCI3XXX_SINGLE				0
#define APCI3XXX_DIFF				1
#define APCI3XXX_CONFIGURATION			0

#define APCI3XXX_TTL_INIT_DIRECTION_PORT2	0

/*
+----------------------------------------------------------------------------+
|                         ANALOG INPUT FUNCTIONS                             |
+----------------------------------------------------------------------------+
*/

/*
+----------------------------------------------------------------------------+
| Function Name     : int   i_APCI3XXX_TestConversionStarted                 |
|                          (struct comedi_device    *dev)                           |
+----------------------------------------------------------------------------+
| Task                Test if any conversion started                         |
+----------------------------------------------------------------------------+
| Input Parameters  : -                                                      |
+----------------------------------------------------------------------------+
| Output Parameters : -                                                      |
+----------------------------------------------------------------------------+
| Return Value      : 0 : Conversion not started                             |
|                     1 : Conversion started                                 |
+----------------------------------------------------------------------------+
*/
static int i_APCI3XXX_TestConversionStarted(struct comedi_device *dev)
{
	struct apci3xxx_private *devpriv = dev->private;

	if ((readl(devpriv->dw_AiBase + 8) & 0x80000UL) == 0x80000UL)
		return 1;
	else
		return 0;

}

/*
+----------------------------------------------------------------------------+
| Function Name     : int   i_APCI3XXX_AnalogInputConfigOperatingMode        |
|                          (struct comedi_device    *dev,                           |
|                           struct comedi_subdevice *s,                             |
|                           struct comedi_insn      *insn,                          |
|                           unsigned int         *data)                          |
+----------------------------------------------------------------------------+
| Task           Converting mode and convert time selection                  |
+----------------------------------------------------------------------------+
| Input Parameters  : b_SingleDiff  = (unsigned char)  data[1];                       |
|                     b_TimeBase    = (unsigned char)  data[2]; (0: ns, 1:micros 2:ms)|
|                    dw_ReloadValue = (unsigned int) data[3];                       |
|                     ........                                               |
+----------------------------------------------------------------------------+
| Output Parameters : -                                                      |
+----------------------------------------------------------------------------+
| Return Value      :>0 : No error                                           |
|                    -1 : Single/Diff selection error                        |
|                    -2 : Convert time base unity selection error            |
|                    -3 : Convert time value selection error                 |
|                    -10: Any conversion started                             |
|                    ....                                                    |
|                    -100 : Config command error                             |
|                    -101 : Data size error                                  |
+----------------------------------------------------------------------------+
*/
static int i_APCI3XXX_AnalogInputConfigOperatingMode(struct comedi_device *dev,
						     struct comedi_subdevice *s,
						     struct comedi_insn *insn,
						     unsigned int *data)
{
	const struct apci3xxx_boardinfo *this_board = comedi_board(dev);
	struct apci3xxx_private *devpriv = dev->private;
	int i_ReturnValue = insn->n;
	unsigned char b_TimeBase = 0;
	unsigned char b_SingleDiff = 0;
	unsigned int dw_ReloadValue = 0;
	unsigned int dw_TestReloadValue = 0;

	/************************/
	/* Test the buffer size */
	/************************/

	if (insn->n == 4) {
	   /****************************/
		/* Get the Singel/Diff flag */
	   /****************************/

		b_SingleDiff = (unsigned char) data[1];

	   /****************************/
		/* Get the time base unitiy */
	   /****************************/

		b_TimeBase = (unsigned char) data[2];

	   /*************************************/
		/* Get the convert time reload value */
	   /*************************************/

		dw_ReloadValue = (unsigned int) data[3];

	   /**********************/
		/* Test the time base */
	   /**********************/

		if ((this_board->b_AvailableConvertUnit & (1 << b_TimeBase)) !=
			0) {
	      /*******************************/
			/* Test the convert time value */
	      /*******************************/

			if (dw_ReloadValue <= 65535) {
				dw_TestReloadValue = dw_ReloadValue;

				if (b_TimeBase == 1) {
					dw_TestReloadValue =
						dw_TestReloadValue * 1000UL;
				}
				if (b_TimeBase == 2) {
					dw_TestReloadValue =
						dw_TestReloadValue * 1000000UL;
				}

		 /*******************************/
				/* Test the convert time value */
		 /*******************************/

				if (dw_TestReloadValue >=
				    this_board->ui_MinAcquisitiontimeNs) {
					if ((b_SingleDiff == APCI3XXX_SINGLE)
						|| (b_SingleDiff ==
							APCI3XXX_DIFF)) {
						if (((b_SingleDiff == APCI3XXX_SINGLE)
						        && (this_board->i_NbrAiChannel == 0))
						    || ((b_SingleDiff == APCI3XXX_DIFF)
							&& (this_board->i_NbrAiChannelDiff == 0))
						    ) {
			   /*******************************/
							/* Single/Diff selection error */
			   /*******************************/

							printk("Single/Diff selection error\n");
							i_ReturnValue = -1;
						} else {
			   /**********************************/
							/* Test if conversion not started */
			   /**********************************/

							if (i_APCI3XXX_TestConversionStarted(dev) == 0) {
								devpriv->
									ui_EocEosConversionTime
									=
									(unsigned int)
									dw_ReloadValue;
								devpriv->
									b_EocEosConversionTimeBase
									=
									b_TimeBase;
								devpriv->
									b_SingelDiff
									=
									b_SingleDiff;
								devpriv->
									b_AiInitialisation
									= 1;

			      /*******************************/
								/* Set the convert timing unit */
			      /*******************************/

								writel((unsigned int)b_TimeBase,
									devpriv->dw_AiBase + 36);

			      /**************************/
								/* Set the convert timing */
			      /*************************/

								writel(dw_ReloadValue, devpriv->dw_AiBase + 32);
							} else {
			      /**************************/
								/* Any conversion started */
			      /**************************/

								printk("Any conversion started\n");
								i_ReturnValue =
									-10;
							}
						}
					} else {
		       /*******************************/
						/* Single/Diff selection error */
		       /*******************************/

						printk("Single/Diff selection error\n");
						i_ReturnValue = -1;
					}
				} else {
		    /************************/
					/* Time selection error */
		    /************************/

					printk("Convert time value selection error\n");
					i_ReturnValue = -3;
				}
			} else {
		 /************************/
				/* Time selection error */
		 /************************/

				printk("Convert time value selection error\n");
				i_ReturnValue = -3;
			}
		} else {
	      /*****************************/
			/* Time base selection error */
	      /*****************************/

			printk("Convert time base unity selection error\n");
			i_ReturnValue = -2;
		}
	} else {
	   /*******************/
		/* Data size error */
	   /*******************/

		printk("Buffer size error\n");
		i_ReturnValue = -101;
	}

	return i_ReturnValue;
}

/*
+----------------------------------------------------------------------------+
| Function Name     : int   i_APCI3XXX_InsnConfigAnalogInput                 |
|                          (struct comedi_device    *dev,                           |
|                           struct comedi_subdevice *s,                             |
|                           struct comedi_insn      *insn,                          |
|                           unsigned int         *data)                          |
+----------------------------------------------------------------------------+
| Task           Converting mode and convert time selection                  |
+----------------------------------------------------------------------------+
| Input Parameters  : b_ConvertMode = (unsigned char)  data[0];                       |
|                     b_TimeBase    = (unsigned char)  data[1]; (0: ns, 1:micros 2:ms)|
|                    dw_ReloadValue = (unsigned int) data[2];                       |
|                     ........                                               |
+----------------------------------------------------------------------------+
| Output Parameters : -                                                      |
+----------------------------------------------------------------------------+
| Return Value      :>0: No error                                            |
|                    ....                                                    |
|                    -100 : Config command error                             |
|                    -101 : Data size error                                  |
+----------------------------------------------------------------------------+
*/
static int i_APCI3XXX_InsnConfigAnalogInput(struct comedi_device *dev,
					    struct comedi_subdevice *s,
					    struct comedi_insn *insn,
					    unsigned int *data)
{
	int i_ReturnValue = insn->n;

	/************************/
	/* Test the buffer size */
	/************************/

	if (insn->n >= 1) {
		switch ((unsigned char) data[0]) {
		case APCI3XXX_CONFIGURATION:
			i_ReturnValue =
				i_APCI3XXX_AnalogInputConfigOperatingMode(dev,
				s, insn, data);
			break;

		default:
			i_ReturnValue = -100;
			printk("Config command error %d\n", data[0]);
			break;
		}
	} else {
	   /*******************/
		/* Data size error */
	   /*******************/

		printk("Buffer size error\n");
		i_ReturnValue = -101;
	}

	return i_ReturnValue;
}

/*
+----------------------------------------------------------------------------+
| Function Name     : int   i_APCI3XXX_InsnReadAnalogInput                   |
|                          (struct comedi_device    *dev,                           |
|                           struct comedi_subdevice *s,                             |
|                           struct comedi_insn      *insn,                          |
|                           unsigned int         *data)                          |
+----------------------------------------------------------------------------+
| Task                Read 1 analog input                                    |
+----------------------------------------------------------------------------+
| Input Parameters  : b_Range             = CR_RANGE(insn->chanspec);        |
|                     b_Channel           = CR_CHAN(insn->chanspec);         |
|                     dw_NbrOfAcquisition = insn->n;                         |
+----------------------------------------------------------------------------+
| Output Parameters : -                                                      |
+----------------------------------------------------------------------------+
| Return Value      :>0: No error                                            |
|                    -3 : Channel selection error                            |
|                    -4 : Configuration selelection error                    |
|                    -10: Any conversion started                             |
|                    ....                                                    |
|                    -100 : Config command error                             |
|                    -101 : Data size error                                  |
+----------------------------------------------------------------------------+
*/
static int i_APCI3XXX_InsnReadAnalogInput(struct comedi_device *dev,
					  struct comedi_subdevice *s,
					  struct comedi_insn *insn,
					  unsigned int *data)
{
	const struct apci3xxx_boardinfo *this_board = comedi_board(dev);
	struct apci3xxx_private *devpriv = dev->private;
	int i_ReturnValue = insn->n;
	unsigned char b_Configuration = (unsigned char) CR_RANGE(insn->chanspec);
	unsigned char b_Channel = (unsigned char) CR_CHAN(insn->chanspec);
	unsigned int dw_Temp = 0;
	unsigned int dw_Configuration = 0;
	unsigned int dw_AcquisitionCpt = 0;
	unsigned char b_Interrupt = 0;

	/*************************************/
	/* Test if operating mode configured */
	/*************************************/

	if (devpriv->b_AiInitialisation) {
	   /***************************/
		/* Test the channel number */
	   /***************************/

		if (((b_Channel < this_board->i_NbrAiChannel)
				&& (devpriv->b_SingelDiff == APCI3XXX_SINGLE))
			|| ((b_Channel < this_board->i_NbrAiChannelDiff)
				&& (devpriv->b_SingelDiff == APCI3XXX_DIFF))) {
	      /**********************************/
			/* Test the channel configuration */
	      /**********************************/

			if (b_Configuration > 7) {
		 /***************************/
				/* Channel not initialised */
		 /***************************/

				i_ReturnValue = -4;
				printk("Channel %d range %d selection error\n",
					b_Channel, b_Configuration);
			}
		} else {
	      /***************************/
			/* Channel selection error */
	      /***************************/

			i_ReturnValue = -3;
			printk("Channel %d selection error\n", b_Channel);
		}

	   /**************************/
		/* Test if no error occur */
	   /**************************/

		if (i_ReturnValue >= 0) {
	      /************************/
			/* Test the buffer size */
	      /************************/

			if ((b_Interrupt != 0) || ((b_Interrupt == 0)
					&& (insn->n >= 1))) {
		 /**********************************/
				/* Test if conversion not started */
		 /**********************************/

				if (i_APCI3XXX_TestConversionStarted(dev) == 0) {
		    /******************/
					/* Clear the FIFO */
		    /******************/

					writel(0x10000UL, devpriv->dw_AiBase + 12);

		    /*******************************/
					/* Get and save the delay mode */
		    /*******************************/

					dw_Temp = readl(devpriv->dw_AiBase + 4);
					dw_Temp = dw_Temp & 0xFFFFFEF0UL;

		    /***********************************/
					/* Channel configuration selection */
		    /***********************************/

					writel(dw_Temp, devpriv->dw_AiBase + 4);

		    /**************************/
					/* Make the configuration */
		    /**************************/

					dw_Configuration =
						(b_Configuration & 3) |
						((unsigned int) (b_Configuration >> 2)
						<< 6) | ((unsigned int) devpriv->
						b_SingelDiff << 7);

		    /***************************/
					/* Write the configuration */
		    /***************************/

					writel(dw_Configuration,
					       devpriv->dw_AiBase + 0);

		    /*********************/
					/* Channel selection */
		    /*********************/

					writel(dw_Temp | 0x100UL,
					       devpriv->dw_AiBase + 4);
					writel((unsigned int) b_Channel,
					       devpriv->dw_AiBase + 0);

		    /***********************/
					/* Restaure delay mode */
		    /***********************/

					writel(dw_Temp, devpriv->dw_AiBase + 4);

		    /***********************************/
					/* Set the number of sequence to 1 */
		    /***********************************/

					writel(1, devpriv->dw_AiBase + 48);

		    /***************************/
					/* Save the interrupt flag */
		    /***************************/

					devpriv->b_EocEosInterrupt =
						b_Interrupt;

		    /*******************************/
					/* Save the number of channels */
		    /*******************************/

					devpriv->ui_AiNbrofChannels = 1;

		    /******************************/
					/* Test if interrupt not used */
		    /******************************/

					if (b_Interrupt == 0) {
						for (dw_AcquisitionCpt = 0;
							dw_AcquisitionCpt <
							insn->n;
							dw_AcquisitionCpt++) {
			  /************************/
							/* Start the conversion */
			  /************************/

							writel(0x80000UL, devpriv->dw_AiBase + 8);

			  /****************/
							/* Wait the EOS */
			  /****************/

							do {
								dw_Temp = readl(devpriv->dw_AiBase + 20);
								dw_Temp = dw_Temp & 1;
							} while (dw_Temp != 1);

			  /*************************/
							/* Read the analog value */
			  /*************************/

							data[dw_AcquisitionCpt] = (unsigned int)readl(devpriv->dw_AiBase + 28);
						}
					} else {
		       /************************/
						/* Start the conversion */
		       /************************/

						writel(0x180000UL, devpriv->dw_AiBase + 8);
					}
				} else {
		    /**************************/
					/* Any conversion started */
		    /**************************/

					printk("Any conversion started\n");
					i_ReturnValue = -10;
				}
			} else {
		 /*******************/
				/* Data size error */
		 /*******************/

				printk("Buffer size error\n");
				i_ReturnValue = -101;
			}
		}
	} else {
	   /***************************/
		/* Channel selection error */
	   /***************************/

		printk("Operating mode not configured\n");
		i_ReturnValue = -1;
	}
	return i_ReturnValue;
}

static int i_APCI3XXX_InsnWriteAnalogOutput(struct comedi_device *dev,
					    struct comedi_subdevice *s,
					    struct comedi_insn *insn,
					    unsigned int *data)
{
	struct apci3xxx_private *devpriv = dev->private;
	unsigned int chan = CR_CHAN(insn->chanspec);
	unsigned int range = CR_RANGE(insn->chanspec);
	unsigned int status;
	int i;

	for (i = 0; i < insn->n; i++) {
		/* Set the range selection */
		writel(range, devpriv->dw_AiBase + 96);

		/* Write the analog value to the selected channel */
		writel((data[i] << 8) | chan, devpriv->dw_AiBase + 100);

		/* Wait the end of transfer */
		do {
			status = readl(devpriv->dw_AiBase + 96);
		} while ((status & 0x100) != 0x100);
	}

	return insn->n;
}

/*
+----------------------------------------------------------------------------+
|                              TTL FUNCTIONS                                 |
+----------------------------------------------------------------------------+
*/

/*
+----------------------------------------------------------------------------+
| Function Name     : int   i_APCI3XXX_InsnConfigInitTTLIO                   |
|                          (struct comedi_device    *dev,                           |
|                           struct comedi_subdevice *s,                             |
|                           struct comedi_insn      *insn,                          |
|                           unsigned int         *data)                          |
+----------------------------------------------------------------------------+
| Task           You must calling this function be                           |
|                for you call any other function witch access of TTL.        |
|                APCI3XXX_TTL_INIT_DIRECTION_PORT2(user inputs for direction)|
+----------------------------------------------------------------------------+
| Input Parameters  : b_InitType    = (unsigned char) data[0];                        |
|                     b_Port2Mode   = (unsigned char) data[1];                        |
+----------------------------------------------------------------------------+
| Output Parameters : -                                                      |
+----------------------------------------------------------------------------+
| Return Value      :>0: No error                                            |
|                    -1: Port 2 mode selection is wrong                      |
|                    ....                                                    |
|                    -100 : Config command error                             |
|                    -101 : Data size error                                  |
+----------------------------------------------------------------------------+
*/
static int i_APCI3XXX_InsnConfigInitTTLIO(struct comedi_device *dev,
					  struct comedi_subdevice *s,
					  struct comedi_insn *insn,
					  unsigned int *data)
{
	struct apci3xxx_private *devpriv = dev->private;
	int i_ReturnValue = insn->n;
	unsigned char b_Command = 0;

	/************************/
	/* Test the buffer size */
	/************************/

	if (insn->n >= 1) {
	   /*******************/
		/* Get the command */
		/* **************** */

		b_Command = (unsigned char) data[0];

	   /********************/
		/* Test the command */
	   /********************/

		if (b_Command == APCI3XXX_TTL_INIT_DIRECTION_PORT2) {
	      /***************************************/
			/* Test the initialisation buffer size */
	      /***************************************/

			if ((b_Command == APCI3XXX_TTL_INIT_DIRECTION_PORT2)
				&& (insn->n != 2)) {
		 /*******************/
				/* Data size error */
		 /*******************/

				printk("Buffer size error\n");
				i_ReturnValue = -101;
			}
		} else {
	      /************************/
			/* Config command error */
	      /************************/

			printk("Command selection error\n");
			i_ReturnValue = -100;
		}
	} else {
	   /*******************/
		/* Data size error */
	   /*******************/

		printk("Buffer size error\n");
		i_ReturnValue = -101;
	}

	/*********************************************************************************/
	/* Test if no error occur and APCI3XXX_TTL_INIT_DIRECTION_PORT2 command selected */
	/*********************************************************************************/

	if ((i_ReturnValue >= 0)
		&& (b_Command == APCI3XXX_TTL_INIT_DIRECTION_PORT2)) {
	   /**********************/
		/* Test the direction */
	   /**********************/

		if ((data[1] == 0) || (data[1] == 0xFF)) {
	      /**************************/
			/* Save the configuration */
	      /**************************/

			devpriv->ul_TTLPortConfiguration[0] =
				devpriv->ul_TTLPortConfiguration[0] | data[1];
		} else {
	      /************************/
			/* Port direction error */
	      /************************/

			printk("Port 2 direction selection error\n");
			i_ReturnValue = -1;
		}
	}

	/**************************/
	/* Test if no error occur */
	/**************************/

	if (i_ReturnValue >= 0) {
	   /***********************************/
		/* Test if TTL port initilaisation */
	   /***********************************/

		if (b_Command == APCI3XXX_TTL_INIT_DIRECTION_PORT2) {
	      /*************************/
			/* Set the configuration */
	      /*************************/

			outl(data[1], devpriv->iobase + 224);
		}
	}

	return i_ReturnValue;
}

/*
+----------------------------------------------------------------------------+
|                        TTL INPUT FUNCTIONS                                 |
+----------------------------------------------------------------------------+
*/

/*
+----------------------------------------------------------------------------+
| Function Name     : int     i_APCI3XXX_InsnBitsTTLIO                       |
|                          (struct comedi_device    *dev,                           |
|                           struct comedi_subdevice *s,                             |
|                           struct comedi_insn      *insn,                          |
|                           unsigned int         *data)                          |
+----------------------------------------------------------------------------+
| Task              : Write the selected output mask and read the status from|
|                     all TTL channles                                       |
+----------------------------------------------------------------------------+
| Input Parameters  : dw_ChannelMask = data [0];                             |
|                     dw_BitMask     = data [1];                             |
+----------------------------------------------------------------------------+
| Output Parameters : data[1] : All TTL channles states                      |
+----------------------------------------------------------------------------+
| Return Value      : >0  : No error                                         |
|                    -4   : Channel mask error                               |
|                    -101 : Data size error                                  |
+----------------------------------------------------------------------------+
*/
static int i_APCI3XXX_InsnBitsTTLIO(struct comedi_device *dev,
				    struct comedi_subdevice *s,
				    struct comedi_insn *insn,
				    unsigned int *data)
{
	struct apci3xxx_private *devpriv = dev->private;
	int i_ReturnValue = insn->n;
	unsigned char b_ChannelCpt = 0;
	unsigned int dw_ChannelMask = 0;
	unsigned int dw_BitMask = 0;
	unsigned int dw_Status = 0;

	/************************/
	/* Test the buffer size */
	/************************/

	if (insn->n >= 2) {
	   /*******************************/
		/* Get the channe and bit mask */
	   /*******************************/

		dw_ChannelMask = data[0];
		dw_BitMask = data[1];

	   /*************************/
		/* Test the channel mask */
	   /*************************/

		if (((dw_ChannelMask & 0XFF00FF00) == 0) &&
			(((devpriv->ul_TTLPortConfiguration[0] & 0xFF) == 0xFF)
				|| (((devpriv->ul_TTLPortConfiguration[0] &
							0xFF) == 0)
					&& ((dw_ChannelMask & 0XFF0000) ==
						0)))) {
	      /*********************************/
			/* Test if set/reset any channel */
	      /*********************************/

			if (dw_ChannelMask) {
		 /****************************************/
				/* Test if set/rest any port 0 channels */
		 /****************************************/

				if (dw_ChannelMask & 0xFF) {
		    /*******************************************/
					/* Read port 0 (first digital output port) */
		    /*******************************************/

					dw_Status = inl(devpriv->iobase + 80);

					for (b_ChannelCpt = 0; b_ChannelCpt < 8;
						b_ChannelCpt++) {
						if ((dw_ChannelMask >>
								b_ChannelCpt) &
							1) {
							dw_Status =
								(dw_Status &
								(0xFF - (1 << b_ChannelCpt))) | (dw_BitMask & (1 << b_ChannelCpt));
						}
					}

					outl(dw_Status, devpriv->iobase + 80);
				}

		 /****************************************/
				/* Test if set/rest any port 2 channels */
		 /****************************************/

				if (dw_ChannelMask & 0xFF0000) {
					dw_BitMask = dw_BitMask >> 16;
					dw_ChannelMask = dw_ChannelMask >> 16;

		    /********************************************/
					/* Read port 2 (second digital output port) */
		    /********************************************/

					dw_Status = inl(devpriv->iobase + 112);

					for (b_ChannelCpt = 0; b_ChannelCpt < 8;
						b_ChannelCpt++) {
						if ((dw_ChannelMask >>
								b_ChannelCpt) &
							1) {
							dw_Status =
								(dw_Status &
								(0xFF - (1 << b_ChannelCpt))) | (dw_BitMask & (1 << b_ChannelCpt));
						}
					}

					outl(dw_Status, devpriv->iobase + 112);
				}
			}

	      /*******************************************/
			/* Read port 0 (first digital output port) */
	      /*******************************************/

			data[1] = inl(devpriv->iobase + 80);

	      /******************************************/
			/* Read port 1 (first digital input port) */
	      /******************************************/

			data[1] = data[1] | (inl(devpriv->iobase + 64) << 8);

	      /************************/
			/* Test if port 2 input */
	      /************************/

			if ((devpriv->ul_TTLPortConfiguration[0] & 0xFF) == 0) {
				data[1] =
					data[1] | (inl(devpriv->iobase +
						96) << 16);
			} else {
				data[1] =
					data[1] | (inl(devpriv->iobase +
						112) << 16);
			}
		} else {
	      /************************/
			/* Config command error */
	      /************************/

			printk("Channel mask error\n");
			i_ReturnValue = -4;
		}
	} else {
	   /*******************/
		/* Data size error */
	   /*******************/

		printk("Buffer size error\n");
		i_ReturnValue = -101;
	}

	return i_ReturnValue;
}

/*
+----------------------------------------------------------------------------+
| Function Name     : int i_APCI3XXX_InsnReadTTLIO                           |
|                          (struct comedi_device    *dev,                           |
|                           struct comedi_subdevice *s,                             |
|                           struct comedi_insn      *insn,                          |
|                           unsigned int         *data)                          |
+----------------------------------------------------------------------------+
| Task              : Read the status from selected channel                  |
+----------------------------------------------------------------------------+
| Input Parameters  : b_Channel = CR_CHAN(insn->chanspec)                    |
+----------------------------------------------------------------------------+
| Output Parameters : data[0] : Selected TTL channel state                   |
+----------------------------------------------------------------------------+
| Return Value      : 0   : No error                                         |
|                    -3   : Channel selection error                          |
|                    -101 : Data size error                                  |
+----------------------------------------------------------------------------+
*/
static int i_APCI3XXX_InsnReadTTLIO(struct comedi_device *dev,
				    struct comedi_subdevice *s,
				    struct comedi_insn *insn,
				    unsigned int *data)
{
	struct apci3xxx_private *devpriv = dev->private;
	unsigned char b_Channel = (unsigned char) CR_CHAN(insn->chanspec);
	int i_ReturnValue = insn->n;
	unsigned int *pls_ReadData = data;

	/************************/
	/* Test the buffer size */
	/************************/

	if (insn->n >= 1) {
	   /***********************/
		/* Test if read port 0 */
	   /***********************/

		if (b_Channel < 8) {
	      /*******************************************/
			/* Read port 0 (first digital output port) */
	      /*******************************************/

			pls_ReadData[0] = inl(devpriv->iobase + 80);
			pls_ReadData[0] = (pls_ReadData[0] >> b_Channel) & 1;
		} else {
	      /***********************/
			/* Test if read port 1 */
	      /***********************/

			if ((b_Channel > 7) && (b_Channel < 16)) {
		 /******************************************/
				/* Read port 1 (first digital input port) */
		 /******************************************/

				pls_ReadData[0] = inl(devpriv->iobase + 64);
				pls_ReadData[0] =
					(pls_ReadData[0] >> (b_Channel -
						8)) & 1;
			} else {
		 /***********************/
				/* Test if read port 2 */
		 /***********************/

				if ((b_Channel > 15) && (b_Channel < 24)) {
		    /************************/
					/* Test if port 2 input */
		    /************************/

					if ((devpriv->ul_TTLPortConfiguration[0]
							& 0xFF) == 0) {
						pls_ReadData[0] =
							inl(devpriv->iobase +
							96);
						pls_ReadData[0] =
							(pls_ReadData[0] >>
							(b_Channel - 16)) & 1;
					} else {
						pls_ReadData[0] =
							inl(devpriv->iobase +
							112);
						pls_ReadData[0] =
							(pls_ReadData[0] >>
							(b_Channel - 16)) & 1;
					}
				} else {
		    /***************************/
					/* Channel selection error */
		    /***************************/

					i_ReturnValue = -3;
					printk("Channel %d selection error\n",
						b_Channel);
				}
			}
		}
	} else {
	   /*******************/
		/* Data size error */
	   /*******************/

		printk("Buffer size error\n");
		i_ReturnValue = -101;
	}

	return i_ReturnValue;
}

/*
+----------------------------------------------------------------------------+
|                        TTL OUTPUT FUNCTIONS                                |
+----------------------------------------------------------------------------+
*/

/*
+----------------------------------------------------------------------------+
| Function Name     : int     i_APCI3XXX_InsnWriteTTLIO                      |
|                          (struct comedi_device    *dev,                           |
|                           struct comedi_subdevice *s,                             |
|                           struct comedi_insn      *insn,                          |
|                           unsigned int         *data)                          |
+----------------------------------------------------------------------------+
| Task              : Set the state from TTL output channel                  |
+----------------------------------------------------------------------------+
| Input Parameters  : b_Channel = CR_CHAN(insn->chanspec)                    |
|                     b_State   = data [0]                                   |
+----------------------------------------------------------------------------+
| Output Parameters : -                                                      |
+----------------------------------------------------------------------------+
| Return Value      : 0   : No error                                         |
|                    -3   : Channel selection error                          |
|                    -101 : Data size error                                  |
+----------------------------------------------------------------------------+
*/
static int i_APCI3XXX_InsnWriteTTLIO(struct comedi_device *dev,
				     struct comedi_subdevice *s,
				     struct comedi_insn *insn,
				     unsigned int *data)
{
	struct apci3xxx_private *devpriv = dev->private;
	int i_ReturnValue = insn->n;
	unsigned char b_Channel = (unsigned char) CR_CHAN(insn->chanspec);
	unsigned char b_State = 0;
	unsigned int dw_Status = 0;

	/************************/
	/* Test the buffer size */
	/************************/

	if (insn->n >= 1) {
		b_State = (unsigned char) data[0];

	   /***********************/
		/* Test if read port 0 */
	   /***********************/

		if (b_Channel < 8) {
	      /*****************************************************************************/
			/* Read port 0 (first digital output port) and set/reset the selected channel */
	      /*****************************************************************************/

			dw_Status = inl(devpriv->iobase + 80);
			dw_Status =
				(dw_Status & (0xFF -
					(1 << b_Channel))) | ((b_State & 1) <<
				b_Channel);
			outl(dw_Status, devpriv->iobase + 80);
		} else {
	      /***********************/
			/* Test if read port 2 */
	      /***********************/

			if ((b_Channel > 15) && (b_Channel < 24)) {
		 /*************************/
				/* Test if port 2 output */
		 /*************************/

				if ((devpriv->ul_TTLPortConfiguration[0] & 0xFF)
					== 0xFF) {
		    /*****************************************************************************/
					/* Read port 2 (first digital output port) and set/reset the selected channel */
		    /*****************************************************************************/

					dw_Status = inl(devpriv->iobase + 112);
					dw_Status =
						(dw_Status & (0xFF -
							(1 << (b_Channel -
									16)))) |
						((b_State & 1) << (b_Channel -
							16));
					outl(dw_Status, devpriv->iobase + 112);
				} else {
		    /***************************/
					/* Channel selection error */
		    /***************************/

					i_ReturnValue = -3;
					printk("Channel %d selection error\n",
						b_Channel);
				}
			} else {
		 /***************************/
				/* Channel selection error */
		 /***************************/

				i_ReturnValue = -3;
				printk("Channel %d selection error\n",
					b_Channel);
			}
		}
	} else {
	   /*******************/
		/* Data size error */
	   /*******************/

		printk("Buffer size error\n");
		i_ReturnValue = -101;
	}

	return i_ReturnValue;
}

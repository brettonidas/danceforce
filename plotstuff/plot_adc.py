import serial
import argparse
import numpy as np
import matplotlib.pyplot as plt
from tqdm import tqdm
import pickle

def main(device, polPer, colTime, outputFile, inputFile, absThresh, relThresh):
    
    if inputFile:
        with open(inputFile, 'rb') as f:
            adcVals, t = pickle.load(f)
        nButtons = adcVals.shape[1]
    else: 
        samples = []
        nSamples = int(colTime / polPer)
        with serial.Serial(device) as ser:
            # Throw out the first line, just in case it's half of a line.
            ser.readline()
            for i in tqdm(range(nSamples)):
                s = ser.readline()
                samples.append(s)

        nButtons = 6

        adcVals = np.empty((nSamples, nButtons), dtype=np.int32)
        t = np.empty(nSamples, dtype=np.int32)
        for i, s in enumerate(samples):
            s = s.decode('utf-8')
            s = s[:-2]
            s = s.split(',')
            s = np.asarray(s)
            adcVals[i, :] = s[1:]
            t[i] = s[0]

        t -= np.min(t)

    if outputFile:
        with open(outputFile, 'wb') as f:
            pickle.dump((adcVals, t), f)
    
    plotAdc = False
    if plotAdc:
        plt.figure()
        plt.plot(t, adcVals)
        plt.title('ADC time values')

    plotHistograms = False
    if plotHistograms:
        histBins = 100
        
        nRows = 3
        nCols = 2

        fig, axs = plt.subplots(nRows, nCols)
        plt.title('Sample histograms')
        for i in range(nButtons):
            ax = axs[int(i//nCols), int(i%nCols)]
            
            vals = adcVals[:, i]
            valsRange = np.max(vals) - np.min(vals) + 1
            histBinsThisPlot = min((valsRange, histBins))
            ax.hist(adcVals[:, i], bins=histBinsThisPlot)
    
    ############################################################################
    from cycler import cycler
    color = plt.cm.gist_rainbow(np.linspace(0.1, 0.9, nButtons))
    cyc = cycler(color=color)

    plt.rc('axes', prop_cycle=cyc)
    
    stride = 16
    adcVals = adcVals.astype(np.int32)
    diff = adcVals[:-stride, :] - adcVals[stride:, :]
    
    fig, axs = plt.subplots(3, 1, sharex=True)

    axs[0].plot(t, adcVals)
    axs[0].title.set_text('ADC values')

    axs[1].plot(t[stride:], diff)
    axs[1].title.set_text('ADC dif values')
    
    buttonOn = np.zeros(adcVals.shape, dtype=np.float32)
    buttonOn[adcVals < absThresh] = 0.8
    buttonOn[stride:, :][diff > relThresh] = 0.8
    
    # Offset the plots, so they aren't on top of each other:
    for i in range(buttonOn.shape[1]):
        buttonOn[:, i] += i

    axs[2].plot(t, buttonOn)

    ############################################################################

    plt.show()

if __name__=="__main__":
    p = argparse.ArgumentParser()
    p.add_argument("device", type=str, help="serial port")
    p.add_argument("--polling-period", type=float, default=0.001,
                    help="polling time on the ddr board. default is 1ms.")
    p.add_argument("-t", "--collection-time", type=float, default=10,
                    help="how long to collect data from the board. default is 10 s.")
    p.add_argument("-o", "--output", type=str, default=None,
                    help="file to save the collected data in.")
    p.add_argument("-i", "--input", type=str, default=None,
                    help="Load data from this file instead of collecting from serial port")
    p.add_argument('--abs', type=int, default=400,
                    help='Absolute threshhold. Default=400.')
    p.add_argument('--rel', type=int, default=200,
                    help='Realtive threshold. Default=200.')

    a = p.parse_args()
    main(a.device, a.polling_period, a.collection_time, a.output, a.input, a.abs, a.rel)


using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace NMLS_Graphisch
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }


        static int NumberOfSamples = 1000;
        const int tracking = 40;
        static Stack<double> x = new Stack<double>();
        static Random rnd = new Random();
        static double[] _x = new double[NumberOfSamples];
        static double[,] w = new double[NumberOfSamples, NumberOfSamples];
        static double learnrate = 0.2;
        static double[] pixel_array;
        static int windowSize = 6;

        /*---------------------------------------------------
         *  GO_btn_click()
         *  
         *  starts the predefined algorithem with the specified parameters
         * 
         *---------------------------------------------------*/

        private void GO_btn_Click(object sender, EventArgs e)
        {

            /* Initializing the chart to display all values */
            NumberOfSamples = Int32.Parse(comboBox_pixel.SelectedItem.ToString());
            chart_main.ChartAreas[0].AxisX.Maximum = NumberOfSamples;
            chart_main.ChartAreas[0].AxisY.Maximum = 300;
            chart_main.ChartAreas[0].AxisY.Minimum = -100;

            /* If output is checked weights befor and after the algorithem are printed to a file.*/
            if (checkBox_output.Checked)
            {
                for (int i = 0; i < tracking; i++)
                {
                    for (int k = 0; k < windowSize; k++)
                    {
                        File.AppendAllText("weights.txt",
                            String.Format("[{0}][{1}] {2}\n", k, i, Math.Round(w[k, i], 2).ToString()),
                            Encoding.UTF8);
                    }
                }
            }

            Series[] series = new Series[2];

            /* Deciding which algorithem is going to be calculated */
            switch (comboBox_algorithem.SelectedItem.ToString())
            {

                case "lokaler Mittelwert":
                    series = localMean();
                    break;

                case "direkter Vorgänger":
                    series = directPredecessor();
                    break;

                case "differenzieller Vorgänger":
                    series = diffPredecessor();
                    break;

                default:
                    return;

            }

            /* Adding the series from the algorithems to the chart */
            foreach (Series s in series)
            {
                if (chart_main.Series.IndexOf(s.Name) < 0)
                {
                    chart_main.Series.Add(s);
                }
                else
                {
                    chart_main.Series.RemoveAt(chart_main.Series.IndexOf(s.Name));
                    chart_main.Series.Add(s);
                }
            }

            /* Output weights after algorithem */
            if (checkBox_output.Checked)
            {
                for (int i = 0; i < tracking; i++)
                {
                    for (int k = 0; k < windowSize; k++)
                    {
                        File.AppendAllText("weights_after.txt",
                            String.Format("[{0}][{1}] {2}\n", k, i, Math.Round(w[k, i], 2).ToString()),
                            Encoding.UTF8);
                    }

                }
            }


        }

        /*---------------------------------------------------
         *  localMean()
         * 
         *  the algorithem for the local Mean
         *  
         *  returns a array of Series with the error and prediction for
         *  displaying in a chart
         * 
         *---------------------------------------------------*/

        Series[] localMean()
        {

            int x_count = 0;
            double[] x_error = new double[NumberOfSamples];
            x_error[0] = 0;

            /* Inizilazing series for the main chart */
            Series localMeanError = new Series("Lokaler Mittelwert Error");
            Series localMeanPredict = new Series("Lokaler Mittelwert Prediction");
            localMeanError.ChartType = SeriesChartType.Spline;
            localMeanPredict.ChartType = SeriesChartType.Spline;

            /* Main while loop for the prediction and learing */

            while (x_count < NumberOfSamples - 1)
            {

                /* Initializing a part array of the actual array to capture the last WINDOWSIZE pixels
                 * for predicting and getting the average of the array*/

                double[] x_part_Array = new double[x_count];
                int _sourceIndex = (x_count > windowSize) ? x_count - windowSize : 0;
                int _arrayLength = (x_count > windowSize) ? windowSize : x_count;
                Array.Copy(_x, _sourceIndex, x_part_Array, 0, _arrayLength);
                double x_middle = (x_count > 0) ? (x_part_Array.Sum() / _arrayLength) : 0;


                double x_pred = 0.0;
                double[] x_array = _x;
                double x_actual = _x[x_count + 1];

                /* Prediction algorithem */

                for (int i = 1; i < _arrayLength; i++)
                {
                    x_pred += (w[i, x_count] * (x_array[x_count - i] - x_middle));
                }
                x_pred += x_middle;

                /* If output is checked a file is produced with prediction, actual and the error */

                if (checkBox_output.Checked)
                {
                    File.AppendAllText("lokalerMittelwert.txt",
                   String.Format("{0}. X_pred {1}\n", x_count, x_pred),
                   Encoding.UTF8);

                    File.AppendAllText("lokalerMittelwert.txt",
                        String.Format("{0}. X_actual {1}\n", x_count, x_actual),
                        Encoding.UTF8);
                }

                /* Calculating the error */
                x_error[x_count] = x_actual - x_pred;


                double x_square = 0;

                /* Output */
                if (checkBox_output.Checked)
                {
                    File.AppendAllText("lokalerMittelwert.txt",
                    String.Format("{0}. X_error {1}\n\n", x_count, x_error[x_count]),
                    Encoding.UTF8);
                }

                /* Calulating the LMS value */
                for (int i = 1; i < _arrayLength; i++)
                {
                    x_square += Math.Pow(x_array[x_count - i] - x_middle, 2);
                }

                /* x_square == 0.0, drives to undefined weights therefor we set it to 1 */

                if (x_square == 0.0)
                    x_square = 1.0;

                /* Learning algorithem */
                for (int i = 1; i < _arrayLength; i++)
                {
                    w[i, x_count + 1] = w[i, x_count] + learnrate * x_error[x_count] * ((x_array[x_count - i] - x_middle) / x_square);
                }

                /* Printing error and prediction on the series */

                localMeanError.Points.AddXY(x_count, x_error[x_count]);
                localMeanPredict.Points.AddXY(x_count, x_pred);

                x_count += 1;
            }

            /* Calculating the avearage of the error and the variance of the error */
            double mittel = x_error.Where(d => !double.IsNaN(d)).Sum() / x_error.Length;
            double varianz = 0.0;
            foreach (double x_e in x_error)
            {
                if (!double.IsNaN(x_e))
                    varianz += Math.Pow(x_e - mittel, 2);
            }
            varianz /= x_error.Length;

            /* Displayes both on labels */
            label3.Text = mittel.ToString();
            label5.Text = varianz.ToString();

            /* Output the result */
            if (checkBox_output.Checked)
            {
                File.AppendAllText("ergebnisse.txt",
                        String.Format("Quadratische Varianz(x_error): {0}\n Mittelwert(x_error): {1}\n\n", varianz, mittel),
                        Encoding.UTF8);
            }

            return new Series[] { localMeanPredict, localMeanError };
        }

        /*---------------------------------------------------
         *  directPredecessor()
         * 
         *  the algorithem for the direct predecessor
         *  
         *  returns a array of Series with the error and prediction for
         *  displaying in a chart
         * 
         *---------------------------------------------------*/

        Series[] directPredecessor()
        {
            double[] x_error = new double[NumberOfSamples];
            x_error[0] = 0;
            int x_count = 0;

            /* Inizilazing series for the main chart */
            Series directPredecessorError = new Series("Direkter Vorgänger Error");
            Series directPredecessorPrediction = new Series("Direkter Vorgänger Prediction");
            directPredecessorError.ChartType = SeriesChartType.Spline;
            directPredecessorPrediction.ChartType = SeriesChartType.Spline;

            /* Main while loop for the prediction and learing */
            while (x_count < NumberOfSamples - 1)
            {
                double x_pred = 0.0;
                double x_actual = _x[x_count + 1];

                if (x_count > 0)
                {
                    /* Initializing the windowsize */
                    int _arrayLength = (x_count > windowSize) ? windowSize : x_count;

                    /* Prediction algorithem */
                    for (int i = 1; i < _arrayLength; i++)
                    {
                        x_pred += (w[i, x_count] * (_x[x_count - 1] - _x[x_count - i - 1]));
                    }
                    x_pred += _x[x_count - 1];

                    /* If output is checked a file is produced with prediction, actual and the error */
                    if (checkBox_output.Checked)
                    {
                        File.AppendAllText("direkterVorgaenger.txt",
                       String.Format("{0}. X_pred {1}\n", x_count, x_pred),
                       Encoding.UTF8);

                        File.AppendAllText("direkterVorgaenger.txt",
                            String.Format("{0}. X_actual {1}\n", x_count, x_actual),
                            Encoding.UTF8);
                    }

                    /* Calculating the error */
                    x_error[x_count] = x_actual - x_pred;

                    /* Output */
                    if (checkBox_output.Checked)
                    {
                        File.AppendAllText("direkterVorgaenger.txt",
                        String.Format("{0}. X_error {1}\n\n", x_count, x_error[x_count]),
                        Encoding.UTF8);
                    }

                    double x_square = 0;

                    /* Calulating the LMS value */
                    for (int i = 1; i < _arrayLength; i++)
                    {
                        x_square += Math.Pow(_x[x_count - 1] - _x[x_count - i - 1], 2);
                    }

                    /* x_square == 0.0, drives to undefined weights therefor we set it to 1 */
                    if (x_square == 0.0)
                    {
                        x_square = 1;
                    }

                    /* Learning algorithem */
                    for (int i = 1; i < _arrayLength; i++)
                    {
                        w[i, x_count + 1] = w[i, x_count] + learnrate * x_error[x_count] * ((_x[x_count - 1] - _x[x_count - i - 1]) / x_square);
                    }
                }

                /* Printing error and prediction on the series */
                directPredecessorError.Points.AddXY(x_count, x_error[x_count]);
                directPredecessorPrediction.Points.AddXY(x_count, x_pred);

                x_count += 1;
            }

            /* Calculating the avearage of the error and the variance of the error */
            double mittel = x_error.Where(d => !double.IsNaN(d)).Sum() / x_error.Length;
            double varianz = 0.0;
            foreach (double x_e in x_error)
            {
                if (!double.IsNaN(x_e))
                    varianz += Math.Pow(x_e - mittel, 2);
            }
            varianz /= x_error.Length;

            /* Displayes both on labels */
            label3.Text = mittel.ToString();
            label5.Text = varianz.ToString();

            /* Output the result */
            if (checkBox_output.Checked)
            {
                File.AppendAllText("ergebnisse.txt",
                        String.Format("Quadratische Varianz(x_error): {0}\n Mittelwert(x_error): {1}\n\n", varianz, mittel),
                        Encoding.UTF8);
            }

            return new Series[] { directPredecessorError, directPredecessorPrediction };
        }

        /*---------------------------------------------------
         *  diffPredecessor()
         * 
         *  the algorithem for the differential predecessor
         *  
         *  returns a array of Series with the error and prediction for
         *  displaying in a chart
         * 
         *---------------------------------------------------*/

        Series[] diffPredecessor()
        {
            double[] x_error = new double[NumberOfSamples];
            x_error[0] = 0;
            int x_count = 0;

            /* Inizilazing series for the main chart */
            Series diffPredecessorError = new Series("Differenzieller Vorgänger Error");
            Series diffPredecessorPrediction = new Series("Differenzieller Vorgänger Prediction");
            diffPredecessorError.ChartType = SeriesChartType.Spline;
            diffPredecessorPrediction.ChartType = SeriesChartType.Spline;

            /* Main while loop for the prediction and learing */
            while (x_count < NumberOfSamples - 1)
            {
                double x_pred = 0.0;
                double x_actual = _x[x_count + 1];
                if (x_count > 0)
                {

                    /* Initializing the windowsize */
                    int _arrayLength = (x_count > windowSize) ? windowSize : x_count;

                    /* Prediction algorithem */
                    for (int i = 1; i < _arrayLength; i++)
                    {
                        x_pred += (w[i, x_count] * (_x[x_count - i] - _x[x_count - i - 1]));
                    }
                    x_pred += _x[x_count - 1];

                    /* If output is checked a file is produced with prediction, actual and the error */
                    if (checkBox_output.Checked)
                    {
                        File.AppendAllText("differenziellerVorgaenger.txt",
                       String.Format("{0}. X_pred {1}\n", x_count, x_pred),
                       Encoding.UTF8);

                        File.AppendAllText("differenziellerVorgaenger.txt",
                            String.Format("{0}. X_actual {1}\n", x_count, x_actual),
                            Encoding.UTF8);
                    }

                    /* Calculating the error */
                    x_error[x_count] = x_actual - x_pred;

                    /* Output */
                    if (checkBox_output.Checked)
                    {
                        File.AppendAllText("differenziellerVorgaenger.txt",
                        String.Format("{0}. X_error {1}\n\n", x_count, x_error[x_count]),
                        Encoding.UTF8);
                    }


                    double x_square = 0;

                    /* Calulating the LMS value */
                    for (int i = 1; i < _arrayLength; i++)
                    {
                        x_square += Math.Pow(_x[x_count - i] - _x[x_count - i - 1], 2);
                    }

                    /* x_square == 0.0, drives to undefined weights therefor we set it to 1 */
                    if (x_square == 0.0)
                    {
                        x_square = 1;
                    }

                    /* Learning algorithem */
                    for (int i = 1; i < _arrayLength; i++)
                    {
                        w[i, x_count + 1] = w[i, x_count] + learnrate * x_error[x_count] * ((_x[x_count - i] - _x[x_count - i - 1]) / x_square);
                    }

                }

                /* Printing error and prediction on the series */
                diffPredecessorError.Points.AddXY(x_count, x_error[x_count]);
                diffPredecessorPrediction.Points.AddXY(x_count, x_pred);

                x_count += 1;
            }

            /* Calculating the avearage of the error and the variance of the error */
            double mittel = x_error.Where(d => !double.IsNaN(d)).Sum() / x_error.Length;
            double varianz = 0.0;
            foreach (double x_e in x_error)
            {
                if (!double.IsNaN(x_e))
                    varianz += Math.Pow(x_e - mittel, 2);
            }
            varianz /= x_error.Length;

            /* Displayes both on labels */
            label3.Text = mittel.ToString();
            label5.Text = varianz.ToString();

            /* Output the result */
            if (checkBox_output.Checked)
            {
                File.AppendAllText("ergebnisse.txt",
                        String.Format("Quadratische Varianz(x_error): {0}\n Mittelwert(x_error): {1}\n\n", varianz, mittel),
                        Encoding.UTF8);
            }

            return new Series[] { diffPredecessorError, diffPredecessorPrediction };
        }

        /*---------------------------------------------------
        *  Form1_Load()
        * 
        *  loads the form and initializes some variables
        * 
        *---------------------------------------------------*/
        private void Form1_Load(object sender, EventArgs e)
        {
            comboBox_algorithem.SelectedIndex = 0;
            comboBox_pixel.SelectedIndex = 0;
            chart_main.Series.Clear();
            Series x_actual = new Series("Actual x Value");
            x_actual.ChartType = SeriesChartType.Spline;

            /*  Initializing weights and actual values
                In case no picture is loaded, actual values are generated
                And printing them on a chart */
            for (int i = 0; i < NumberOfSamples; i++)
            {
                _x[i] += ((255.0 / NumberOfSamples) * i);
                for (int k = 0; k < windowSize; k++)
                {
                    w[k, i] = rnd.NextDouble();
                }
                x_actual.Points.AddXY(i, _x[i]);
            }
            chart_main.Series.Add(x_actual);
        }

        /*---------------------------------------------------
        *  Clear_btn_Click()
        * 
        *  clears the chart and reprints the actual values
        * 
        *---------------------------------------------------*/

        private void Clear_btn_Click(object sender, EventArgs e)
        {
            chart_main.Series.Clear();
            Series x_actual = new Series("Actual x Value");
            x_actual.ChartType = SeriesChartType.Spline;
            for (int i = 0; i < Int32.Parse(comboBox_pixel.SelectedItem.ToString()); i++)
            {
                x_actual.Points.AddXY(i, _x[i]);
            }
            chart_main.Series.Add(x_actual);
        }

        /*---------------------------------------------------
        *  Load_btn_Click()
        * 
        *  loads a image from a file and stores all pixels in an array
        * 
        *---------------------------------------------------*/

        private void Load_btn_Click(object sender, EventArgs e)
        {


            OpenFileDialog openFileDialog = new OpenFileDialog();

            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                /* Catch a exception if any is raised */
                try
                {
                    /* Loads file into a bitmap */
                    Bitmap img = new Bitmap(openFileDialog.FileName);
                    pixel_array = new double[img.Width * img.Height];

                    for (int i = 1; i < img.Width; i++)
                    {
                        for (int j = 1; j < img.Height; j++)
                        {

                            Color pixel = img.GetPixel(i, j);
                            pixel_array[j * i] = pixel.R;

                        }
                    }

                    /* If NuberOfSamples is greater then 2147483591 its size is over
                     * the limit of C# possible indexes in an array, so we have to decrease
                     * the size of NumberOfSamples */

                    NumberOfSamples = (img.Width * img.Height) / 2;

                    if (NumberOfSamples > 2147483591 / 5)
                        NumberOfSamples = 2147483591 / 5;

                    /* Add every 10% a number into comboBox_pixel for calculating */
                    for (int i = 1; i < 11; i++)
                        comboBox_pixel.Items.Add((int)(NumberOfSamples * 0.1 * i));

                    _x = pixel_array;

                    /* Recreating the weights with the new size of NumberOfSamples */

                    w = new double[windowSize, NumberOfSamples];
                    for (int i = 0; i < NumberOfSamples; i++)
                    {
                        for (int k = 0; k < windowSize; k++)
                        {
                            w[k, i] = rnd.NextDouble();
                        }
                    }

                    /* Clearing the screen after loading the pixels to make the new graph occure */
                    Clear_btn_Click(new object(), new EventArgs());

                }
                catch (Exception exep)
                {
                    MessageBox.Show("Konnte Bild nicht laden.");
                    MessageBox.Show(String.Format("{0}", exep.ToString()));
                }
            }
        }
    }

}


using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace eg
{
     class Masyvas
    {
        public int[] stekas;
        private int counter;
        private readonly object locker;
        public bool finish = false; 
        public bool printed = false;

        public Masyvas(int n)
        {
            locker = new object();
            counter = 0;
            stekas = new int[n];
        }

        public void push(int i)
        {
            lock (locker)
            {
                stekas[counter++] = i;
                if (counter == 50)
                    finish = true;
            }     
        }

        public void printMax()
        {
            lock (locker)
            {
                int max = -1;
                foreach (var a in stekas)
                {
                    if (a > max)
                    {
                        max = a;
                    }
                }
                Console.WriteLine(max);
            }
        }


    }
}

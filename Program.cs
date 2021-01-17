
using System;
using System.Linq;
using System.Threading;

namespace eg
{
    class Program
    {
        static void Main(string[] args)
        {
            int maxThreads = 10;
            Masyvas st = new Masyvas(maxThreads*5);

            var t = new Thread(() => {
                while(!st.finish)
                {
                    st.printMax(); 
                }
                if(!st.printed)
                    st.printMax();
                });

            var indexes = Enumerable.Range(0, 10);
            var threads = indexes.Select(ind => new Thread(() =>
            {
                for (int i = 0; i < 5; i++)
                {
                    st.push(ind);
                }
            })).ToList();
            threads.Add(t); 

            foreach (var thread in threads)
                thread.Start();
            foreach (var thread in threads)
                thread.Join();
            
            // foreach (var thread in st.stekas)
            //     Console.WriteLine(thread);
            //Console.WriteLine(st.stekas.Count());
        }
    }
}

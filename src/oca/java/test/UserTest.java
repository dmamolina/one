/*******************************************************************************
 * Copyright 2002-2011, OpenNebula Project Leads (OpenNebula.org)
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/
import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.opennebula.client.Client;
import org.opennebula.client.OneResponse;
import org.opennebula.client.user.User;
import org.opennebula.client.user.UserPool;

public class UserTest
{

    private static User     user;
    private static UserPool userPool;

    private static Client client;

    private static OneResponse  res;
    private static String       name      = "new_test_user";
    private static String       password  = "new_test_password";

    /**
     * @throws java.lang.Exception
     */
    @BeforeClass
    public static void setUpBeforeClass() throws Exception
    {
        client      = new Client();
        userPool    = new UserPool(client);
    }

    /**
     * @throws java.lang.Exception
     */
    @AfterClass
    public static void tearDownAfterClass() throws Exception
    {
    }

    /**
     * @throws java.lang.Exception
     */
    @Before
    public void setUp() throws Exception
    {
        res = User.allocate(client, name, password);

        int uid = res.isError() ? -1 : Integer.parseInt(res.getMessage()); 
        user        = new User(uid, client);
    }

    /**
     * @throws java.lang.Exception
     */
    @After
    public void tearDown() throws Exception
    {
        user.delete();
    }


    @Test
    public void allocate()
    {
        userPool.info();

        boolean found = false;
        for(User u : userPool)
        {
            found = found || u.getName().equals(name);
        }

        assertTrue( found );
    }

    @Test
    public void update()
    {
        res = user.info();
        assertTrue( res.getErrorMessage(), !res.isError() );
        
        assertTrue( user.id() >= 0 );
        assertTrue( user.getName().equals(name) );
    }

    @Test
    public void attributes()
    {
        res = user.info();
        assertTrue( res.getErrorMessage(), !res.isError() );

        assertTrue( user.xpath("NAME").equals(name) );
        assertTrue( user.xpath("ENABLED").equals("1") );
    }

    @Test
    public void delete()
    {
        res = user.info();
        assertTrue( res.getErrorMessage(), !res.isError() );
        assertTrue( user.isEnabled() );

        res = user.delete();
        assertTrue( res.getErrorMessage(), !res.isError() );

        res = user.info();
        assertTrue( res.getErrorMessage(), res.isError() );
    }
}
